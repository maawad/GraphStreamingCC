#include <gtest/gtest.h>
#include <cmath>
#include <chrono>
#include <thread>
#include "../include/supernode.h"
#include "../include/graph_worker.h"

const long seed = 7000000001;
const unsigned long long int num_nodes = 2000;

class SupernodeTestSuite : public testing::Test {
protected:
  static vector<Edge>* graph_edges;
  static vector<Edge>* odd_graph_edges;
  static bool* prime;
  static void SetUpTestSuite() {
    srand(1000000007);
    graph_edges = new vector<Edge>();
    odd_graph_edges = new vector<Edge>();
    for (unsigned i=2;i<num_nodes;++i) {
      for (unsigned j = i*2; j < num_nodes; j+=i) {
        graph_edges->push_back({i,j});
        if ((j/i)%2) odd_graph_edges->push_back({i,j});
      }
    }

    // sieve
    prime = (bool*) malloc(num_nodes*sizeof(bool));
    fill(prime,prime+num_nodes,true);
    for (unsigned i = 2; i < num_nodes; i++) {
      if (prime[i]) {
        for (unsigned j = i * i; j < num_nodes; j += i) prime[j] = false;
      }
    }
  }
  static void TearDownTestSuite() {
    delete graph_edges;
    delete odd_graph_edges;
    free(prime);
  }

  void SetUp() override {Supernode::configure(num_nodes);}
  void TearDown() override {}
};

vector<Edge>* SupernodeTestSuite::graph_edges;
vector<Edge>* SupernodeTestSuite::odd_graph_edges;
bool* SupernodeTestSuite::prime;

TEST_F(SupernodeTestSuite, GIVENnoEdgeUpdatesIFsampledTHENnoEdgeIsReturned) {
  Supernode* s = Supernode::makeSupernode(num_nodes, seed);
  boost::optional<Edge> res;
  for (int i=0;i<(int)log2(num_nodes);++i) {
    EXPECT_FALSE(res.is_initialized()) << i << " was inited";
  }
}

TEST_F(SupernodeTestSuite, IFsampledTooManyTimesTHENthrowOutOfQueries) {
  Supernode* s = Supernode::makeSupernode(num_nodes, seed);
  for (int i=0;i<(int)log2(num_nodes);++i) {
    s->sample();
  }
  ASSERT_THROW(s->sample(), OutOfQueriesException);
}

TEST_F(SupernodeTestSuite, TestSampleInsertGrinder) {
  vector<Supernode*> snodes;
  snodes.reserve(num_nodes);
  for (unsigned i = 0; i < num_nodes; ++i)
    snodes[i] = Supernode::makeSupernode(num_nodes, seed);

  // insert all edges
  for (auto edge : *graph_edges) {
    vec_t encoded = nondirectional_non_self_edge_pairing_fn(edge.first, edge
    .second);
    snodes[edge.first]->update(encoded);
    snodes[edge.second]->update(encoded);
  }

  // allow one NoGoodBucket-type failure
  bool second_chance = false;
  boost::optional<Edge> sampled;
  for (unsigned i = 2; i < num_nodes; ++i) {
    for (int j = 0; j < (int) log2(num_nodes); ++j) {
      try {
        sampled = snodes[i]->sample();
        if (i >= num_nodes/2 && prime[i]) {
          ASSERT_FALSE(sampled.is_initialized())
                            << "False positive in sample " << i;
        } else {
          ASSERT_TRUE(sampled.is_initialized())
                            << "False negative in sample " << i;
          ASSERT_TRUE(std::max(sampled->first, sampled->second) % std::min
                (sampled->first, sampled->second) == 0
                      && (i == sampled->first || i == sampled->second)) <<
                      "Failed on {" << sampled->first << "," <<
                      sampled->second << "} with i = " << i;
        }
      } catch (NoGoodBucketException &e) {
        if (second_chance) FAIL() << "2 samplings failed to find a good bucket";
        else second_chance = true;
      }
    }
  }
  for (unsigned i = 0; i < num_nodes; ++i)
    delete snodes[i];
}

TEST_F(SupernodeTestSuite, TestSampleDeleteGrinder) {
  vector<Supernode*> snodes;
  snodes.reserve(num_nodes);
  for (unsigned i = 0; i < num_nodes; ++i)
    snodes[i] = Supernode::makeSupernode(num_nodes, seed);

  // insert all edges
  for (auto edge : *graph_edges) {
    vec_t encoded = nondirectional_non_self_edge_pairing_fn(edge.first, edge
    .second);
    snodes[edge.first]->update(encoded);
    snodes[edge.second]->update(encoded);
  }
  // then remove half of them (odds)
  for (auto edge : *odd_graph_edges) {
    vec_t encoded = nondirectional_non_self_edge_pairing_fn(edge.first, edge
    .second);
    snodes[edge.first]->update(encoded);
    snodes[edge.second]->update(encoded);
  }

  // allow one NoGoodBucket-type failure
  bool second_chance = false;
  boost::optional<Edge> sampled;
  for (unsigned i = 2; i < num_nodes; ++i) {
    for (int j = 0; j < (int) log2(num_nodes); ++j) {
      try {
        sampled = snodes[i]->sample();
        if (i >= num_nodes/2 && i % 2) {
          ASSERT_FALSE(sampled.is_initialized())
                            << "False positive in sample " << i;
        } else {
          ASSERT_TRUE(sampled.is_initialized())
                            << "False negative in sample " << i;
          ASSERT_TRUE(std::max(sampled->first, sampled->second) % std::min
                (sampled->first, sampled->second) == 0
                      && (std::max(sampled->first, sampled->second) / std::min
                (sampled->first, sampled->second)) % 2 == 0
                      && (i == sampled->first || i == sampled->second)) <<
                      "Failed on {" << sampled->first << "," <<
                      sampled->second << "} with i = " << i;
        }
      } catch (NoGoodBucketException &e) {
        if (second_chance) FAIL() << "2 samplings failed to find a good bucket";
        else second_chance = true;
      }
    }
  }
  for (unsigned i = 0; i < num_nodes; ++i)
    delete snodes[i];
}

void inline apply_delta_to_node(Supernode* node, const std::vector<vec_t>& updates) {
  Supernode* loc = (Supernode*) malloc(Supernode::get_size());
  Supernode::delta_supernode(node->n, node->seed, updates, loc);
  node->apply_delta_update(loc);
  free(loc);
}

TEST_F(SupernodeTestSuite, TestBatchUpdate) {
  unsigned long vec_size = 1000000000, num_updates = 100000;
  srand(time(NULL));
  std::vector<vec_t> updates(num_updates);
  for (unsigned long i = 0; i < num_updates; i++) {
    updates[i] = static_cast<vec_t>(rand() % vec_size);
  }
  auto seed = rand();
  Supernode::configure(vec_size);
  Supernode* supernode = Supernode::makeSupernode(vec_size, seed);
  Supernode* supernode_batch = Supernode::makeSupernode(vec_size, seed);
  auto start_time = std::chrono::steady_clock::now();
  for (const auto& update : updates) {
    supernode->update(update);
  }
  std::cout << "One by one updates took " << static_cast<std::chrono::duration<long double>>(std::chrono::steady_clock::now() - start_time).count() << std::endl;
  start_time = std::chrono::steady_clock::now();
  apply_delta_to_node(supernode_batch, updates);
  std::cout << "Batched updates took " << static_cast<std::chrono::duration<long double>>(std::chrono::steady_clock::now() - start_time).count() << std::endl;

  ASSERT_EQ(supernode->logn, supernode_batch->logn);
  ASSERT_EQ(supernode->idx, supernode_batch->idx);
  for (int i=0;i<supernode->logn;++i) {
    ASSERT_EQ(*supernode->get_sketch(i), *supernode_batch->get_sketch(i));
  }
}

TEST_F(SupernodeTestSuite, TestConcurrency) {
  int num_threads_per_group = 2;
  unsigned num_threads =
       std::thread::hardware_concurrency() / num_threads_per_group - 1; // hyperthreading?
  unsigned vec_len = 1000000;
  unsigned num_updates = 100000;
  Supernode::configure(vec_len);

  std::vector<std::vector<vec_t>> test_vec(num_threads,
                                          std::vector<vec_t>(num_updates));
  for (unsigned i = 0; i < num_threads; ++i) {
    for (unsigned long j = 0; j < num_updates; ++j) {
      test_vec[i][j] = static_cast<vec_t>(rand() % vec_len);
    }
  }
  int seed = rand();

  Supernode *supernode = Supernode::makeSupernode(vec_len, seed);
  Supernode *piecemeal = Supernode::makeSupernode(vec_len, seed);

  GraphWorker::set_config(0,num_threads_per_group); // set number of threads per omp parallel

  // concurrently run batch_updates
  std::thread thd[num_threads];
  for (unsigned i = 0; i < num_threads; ++i) {
    thd[i] = std::thread(apply_delta_to_node, piecemeal, std::ref(test_vec[i]));
  }

  // do single-update sketch in the meantime
  for (unsigned i = 0; i < num_threads; ++i) {
    for (unsigned long j = 0; j < num_updates; j++) {
      supernode->update(test_vec[i][j]);
    }
  }

  for (unsigned i = 0; i < num_threads; ++i) {
    thd[i].join();
  }

  for (int i = 0; i < supernode->logn; ++i) {
    ASSERT_EQ(*supernode->get_sketch(i), *piecemeal->get_sketch(i));
  }
}

TEST_F(SupernodeTestSuite, TestSerialization) {
  vector<Supernode*> snodes;
  snodes.reserve(num_nodes);
  for (unsigned i = 0; i < num_nodes; ++i)
    snodes[i] = Supernode::makeSupernode(num_nodes, seed);

  // insert all edges
  for (auto edge : *graph_edges) {
    vec_t encoded = nondirectional_non_self_edge_pairing_fn(edge.first, edge
          .second);
    snodes[edge.first]->update(encoded);
    snodes[edge.second]->update(encoded);
  }

  std::cout << "Finished inserting" << std::endl;
  auto file = std::fstream("./out_supernode.txt", std::ios::out | std::ios::binary);
  snodes[num_nodes/2]->write_binary(file);
  file.close();
  std::cout << "Finished writing" << std::endl;

  auto in_file = std::fstream("./out_supernode.txt", std::ios::in | std::ios::binary);

  Supernode* reheated = Supernode::makeSupernode(num_nodes, seed, in_file);

  for (int i = 0; i < snodes[num_nodes / 2]->logn; ++i) {
    ASSERT_EQ(*snodes[num_nodes / 2]->get_sketch(i), *reheated->get_sketch(i));
  }
}
