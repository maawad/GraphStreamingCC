#include <map>
#include <iostream>
#include <buffer_tree.h>
#include <chrono>
#include <sys/mman.h>

#include "include/graph.h"
#include "include/util.h"
#include "include/graph_worker.h"

std::atomic<uint64_t> Graph::num_updates;

Graph::Graph(uint64_t num_nodes, int factor): num_nodes(num_nodes) {
#ifdef VERIFY_SAMPLES_F
  cout << "Verifying samples..." << endl;
#endif
  Supernode::configure(num_nodes);
  representatives = new set<Node>();
  supernodes = new Supernode*[num_nodes];
  parent = new Node[num_nodes];
  seed = time(nullptr);
  srand(seed);
  for (Node i=0;i<num_nodes;++i) {
    representatives->insert(i);
    supernodes[i] = Supernode::makeSupernode(num_nodes,seed);
    parent[i] = i;
  }
  num_updates = 0;
  std::string buffer_loc_prefix = configure_system(); // read the configuration file to configure the system
#ifdef USE_FBT_F
  // Create buffer tree and start the graphWorkers
  bf = new BufferTree(buffer_loc_prefix, (1<<23), 512, num_nodes, 1, GraphWorker::get_num_groups(), 8, true);
  GraphWorker::start_workers(this, bf, Supernode::get_size());
#else
  // unsigned long node_size = 24*pow((log2(num_nodes)), 3);
  // node_size /= sizeof(node_id_t) * factor; // we want a fourth of a full buffer size
  unsigned long node_size = factor + 2; // alternative when checking buffer size
  wq = new WorkQueue(node_size, num_nodes, 2*GraphWorker::get_num_groups());
  GraphWorker::start_workers(this, wq, Supernode::get_size());
#endif
}

Graph::Graph(const std::string& input_file) {
  double num_bucket_factor;
  auto binary_in = std::fstream(input_file, std::ios::in | std::ios::binary);
  binary_in.read((char*)&seed, sizeof(long));
  binary_in.read((char*)&num_nodes, sizeof(uint64_t));
  binary_in.read((char*)&num_bucket_factor, sizeof(double));
  Supernode::configure(num_nodes, num_bucket_factor);

#ifdef VERIFY_SAMPLES_F
  cout << "Verifying samples..." << endl;
#endif
  representatives = new set<Node>();
  supernodes = new Supernode*[num_nodes];
  parent = new Node[num_nodes];
  for (Node i = 0; i < num_nodes; ++i) {
    representatives->insert(i);
    supernodes[i] = Supernode::makeSupernode(num_nodes, seed, binary_in);
    parent[i] = i;
  }
  binary_in.close();
  num_updates = 0;
  std::string buffer_loc_prefix = configure_system(); // read the configuration file to configure the system
#ifdef USE_FBT_F
  // Create buffer tree and start the graphWorkers
  bf = new BufferTree(buffer_loc_prefix, (1<<23), 512, num_nodes, 1, GraphWorker::get_num_groups(), 8, true);
  GraphWorker::start_workers(this, bf, Supernode::get_size());
#else
  unsigned long node_size = 24 * pow(log2(num_nodes), 3);
  node_size /= sizeof(node_id_t) * 4;
  wq = new WorkQueue(node_size, num_nodes, 2*GraphWorker::get_num_groups());
  GraphWorker::start_workers(this, wq, Supernode::get_size());
#endif
}

Graph::~Graph() {
  for (unsigned i=0;i<num_nodes;++i)
    free(supernodes[i]); // free because memory is malloc'd in make_supernode
  delete[] supernodes;
  delete[] parent;
  delete representatives;
  GraphWorker::stop_workers(); // join the worker threads
#ifdef USE_FBT_F
  delete bf;
#else
  delete wq;
#endif
}

void Graph::update(GraphUpdate upd) {
  if (update_locked) throw UpdateLockedException();
  Edge &edge = upd.first;

#ifdef USE_FBT_F
  bf->insert(edge);
  std::swap(edge.first, edge.second);
  bf->insert(edge);
#else
  wq->insert(edge);
  std::swap(edge.first, edge.second);
  wq->insert(edge);
#endif
}

void Graph::generate_delta_node(uint64_t node_n, long node_seed, uint64_t
							 src, const std::vector<uint64_t>& edges, Supernode *delta_loc) {
  std::vector<vec_t> updates;
  updates.reserve(edges.size());
  for (const auto& edge : edges) {
    if (src < edge) {
      updates.push_back(static_cast<vec_t>(
                            nondirectional_non_self_edge_pairing_fn(src, edge)));
    } else {
      updates.push_back(static_cast<vec_t>(
                            nondirectional_non_self_edge_pairing_fn(edge, src)));
    }
  }
  Supernode::delta_supernode(node_n, node_seed, updates, delta_loc);
}
void Graph::batch_update(uint64_t src, const std::vector<uint64_t>& edges, Supernode *delta_loc) {
  if (update_locked) throw UpdateLockedException();

  num_updates += edges.size();
  generate_delta_node(supernodes[src]->n, supernodes[src]->seed, src, edges, delta_loc);
  supernodes[src]->apply_delta_update(delta_loc);
}

vector<set<Node>> Graph::connected_components() {
#ifdef USE_FBT_F
  bf->force_flush(); // flush everything in buffertree to make final updates
#else
  wq->force_flush();
#endif
  GraphWorker::pause_workers(); // wait for the workers to finish applying the updates
  // after this point all updates have been processed from the buffer tree
  end_time = std::chrono::steady_clock::now();
  // printf("Total number of updates to sketches before CC %lu\n", num_updates.load()); // REMOVE this later

  update_locked = true; // disallow updating the graph after we run the alg
  bool modified;
#ifdef VERIFY_SAMPLES_F
  GraphVerifier verifier {cum_in};
#endif

  do {
    modified = false;
    vector<Node> removed;
    for (Node i: (*representatives)) {
      if (parent[i] != i) continue;
      boost::optional<Edge> edge = supernodes[i]->sample();
#ifdef VERIFY_SAMPLES_F
      if (edge.is_initialized())
        verifier.verify_edge(edge.value());
      else
        verifier.verify_cc(i);
#endif
      if (!edge.is_initialized()) continue;

      Node n;
      // DSU compression
      if (get_parent(edge->first) == i) {
        n = get_parent(edge->second);
        removed.push_back(n);
        parent[n] = i;
      }
      else {
        get_parent(edge->second);
        n = get_parent(edge->first);
        removed.push_back(n);
        parent[n] = i;
      }
      supernodes[i]->merge(*supernodes[n]);
    }
    if (!removed.empty()) modified = true;
    for (Node i : removed) representatives->erase(i);
  } while (modified);

  map<Node, set<Node>> temp;
  for (Node i=0;i<num_nodes;++i)
    temp[get_parent(i)].insert(i);
  vector<set<Node>> retval;
  retval.reserve(temp.size());
  for (const auto& it : temp) retval.push_back(it.second);
#ifdef VERIFY_SAMPLES_F
  verifier.verify_soln(retval);
#endif

  CC_end_time = std::chrono::steady_clock::now();
  return retval;
}

vector<set<Node>> Graph::parallel_connected_components() {
#ifdef USE_FBT_F
  bf->force_flush(); // flush everything in buffertree to make final updates
#else
  wq->force_flush();
#endif
  GraphWorker::pause_workers(); // wait for the workers to finish applying the updates
  end_time = std::chrono::steady_clock::now();
  // after this point all updates have been processed from the buffer tree
  // printf("Total number of updates to sketches before CC %lu\n", num_updates.load()); // REMOVE this later
  update_locked = true; // disallow updating the graph after we run the alg
  bool modified;
#ifdef VERIFY_SAMPLES_F
  GraphVerifier verifier { cum_in };
#endif
  pair<Node,Node> query[num_nodes];
  Node size[num_nodes];
  vector<Node> reps(num_nodes);
  fill(size, size + num_nodes, 1);
  for (Node i = 0; i < num_nodes; ++i) {
    reps[i] = i;
  }

  do {
    modified = false;
    #pragma omp parallel for default(none) shared(query, reps)
    for (Node i = 0; i < reps.size(); ++i) {
      auto edge = supernodes[reps[i]]->sample();
      if (!edge.is_initialized()) {
        query[reps[i]] = {i,i};
        continue;
      }
      query[reps[i]] = edge.get();
    }
    vector<Node> to_remove;
    for (Node i : reps) {
      Node a = get_parent(query[i].first);
      Node b = get_parent(query[i].second);
      if (a == b) continue;
#ifdef VERIFY_SAMPLES_F
      verifier.verify_edge({query[i].first,query[i].second});
#endif

      // make sure a is the one to be merged into
      if (size[a] < size[b]) std::swap(a,b);
      to_remove.push_back(b);
      parent[b] = a;
      size[a] += size[b];
      supernodes[a]->merge(*supernodes[b]);
    }
    if (!to_remove.empty()) modified = true;
    sort(to_remove.begin(), to_remove.end());

    // 2-pointer to find set difference
    vector<Node> temp_diff;
    Node ptr1 = 0;
    Node ptr2 = 0;
    while (ptr1 < reps.size() && ptr2 < to_remove.size()) {
      if (reps[ptr1] == to_remove[ptr2]) {
        ++ ptr1; ++ptr2;
      } else {
        temp_diff.push_back(reps[ptr1]);
        ++ptr1;
      }
    }
    while (ptr1 < reps.size()) {
      temp_diff.push_back(reps[ptr1]);
      ++ptr1;
    }

    swap(reps, temp_diff);
  } while (modified);

  map<Node, set<Node>> temp;
  for (Node i=0;i<num_nodes;++i)
    temp[get_parent(i)].insert(i);
  vector<set<Node>> retval;
  retval.reserve(temp.size());
  for (const auto& it : temp) retval.push_back(it.second);

  CC_end_time = std::chrono::steady_clock::now();
  return retval;
}

void Graph::post_cc_resume() {
  GraphWorker::unpause_workers();
  update_locked = false;
}

Node Graph::get_parent(Node node) {
  if (parent[node] == node) return node;
  return parent[node] = get_parent(parent[node]);
}

void Graph::write_binary(const std::string& filename) {
#ifdef USE_FBT_F
  bf->force_flush(); // flush everything in buffertree to make final updates
#else
  wq->force_flush();
#endif
  GraphWorker::pause_workers(); // wait for the workers to finish applying the updates
  // after this point all updates have been processed from the buffer tree

  auto binary_out = std::fstream(filename, std::ios::out | std::ios::binary);
  double sketch_factor = Sketch::get_bucket_factor();
  binary_out.write((char*)&seed, sizeof(long));
  binary_out.write((char*)&num_nodes, sizeof(uint64_t));
  binary_out.write((char*)&sketch_factor, sizeof(double));
  for (Node i = 0; i < num_nodes; ++i) {
    supernodes[i]->write_binary(binary_out);
  }
  binary_out.close();
}

