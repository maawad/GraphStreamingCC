#include <gtest/gtest.h>
#include <fstream>
#include "../include/graph.h"
#include "../include/test/file_graph_verifier.h"
#include "../include/test/mat_graph_verifier.h"
#include "../include/test/graph_gen.h"
#include "../include/test/write_configuration.h"

/**
 * For many of these tests (especially for those upon very sparse and small graphs)
 * we allow for a certain number of failures per test.
 * This is because the responsibility of these tests is to quickly alert us 
 * to “this code is very wrong” whereas the statistical testing is responsible 
 * for a more fine grained analysis.
 * In this context a false positive is much worse than a false negative.
 * With 2 failures allowed per test our entire testing suite should fail 1/5000 runs.
 */

// We create this class and instantiate a paramaterized test suite so that we
// can run these tests both with the GutterTree and with StandAloneGutters
class GraphTest : public testing::TestWithParam<bool> {

};
INSTANTIATE_TEST_SUITE_P(GraphTestSuite, GraphTest, testing::Values(true, false));

TEST_P(GraphTest, SmallGraphConnectivity) {
  write_configuration(GetParam());
  const std::string fname = __FILE__;
  size_t pos = fname.find_last_of("\\/");
  const std::string curr_dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);
  ifstream in{curr_dir + "/res/multiples_graph_1024.txt"};
  node_t num_nodes;
  in >> num_nodes;
  long m;
  in >> m;
  node_t a, b;
  Graph g{num_nodes};
  while (m--) {
    in >> a >> b;
    g.update({{a, b}, INSERT});
  }
  g.set_verifier(std::make_unique<FileGraphVerifier>(curr_dir + "/res/multiples_graph_1024.txt"));
  ASSERT_EQ(78, g.connected_components().size());
}

TEST_P(GraphTest, IFconnectedComponentsAlgRunTHENupdateLocked) {
  write_configuration(GetParam());
  const std::string fname = __FILE__;
  size_t pos = fname.find_last_of("\\/");
  const std::string curr_dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);
  ifstream in{curr_dir + "/res/multiples_graph_1024.txt"};
  node_t num_nodes;
  in >> num_nodes;
  long m;
  in >> m;
  node_t a, b;
  Graph g{num_nodes};
  while (m--) {
    in >> a >> b;
    g.update({{a, b}, INSERT});
  }
  g.set_verifier(std::make_unique<FileGraphVerifier>(curr_dir + "/res/multiples_graph_1024.txt"));
  g.connected_components();
  ASSERT_THROW(g.update({{1,2}, INSERT}), UpdateLockedException);
  ASSERT_THROW(g.update({{1,2}, DELETE}), UpdateLockedException);
}

TEST_P(GraphTest, TestCorrectnessOnSmallRandomGraphs) {
  write_configuration(GetParam());
  int num_trials = 10;
  int allow_fail = 2; // allow 2 failures
  int fails = 0;
  while (num_trials--) {
    generate_stream();
    ifstream in{"./sample.txt"};
    node_t n, m;
    in >> n >> m;
    Graph g{n};
    int type, a, b;
    while (m--) {
      in >> type >> a >> b;
      if (type == INSERT) {
        g.update({{a, b}, INSERT});
      } else g.update({{a, b}, DELETE});
    }

    g.set_verifier(std::make_unique<FileGraphVerifier>("./cumul_sample.txt"));
    try {
      g.connected_components();
    } catch (OutOfQueriesException& err) {
      fails++;
      if (fails > allow_fail) {
        printf("More than %i failures failing test\n", allow_fail);
        throw;
      }
    }
  }
}

TEST_P(GraphTest, TestCorrectnessOnSmallSparseGraphs) {
  write_configuration(GetParam());
  int num_trials = 10;
  int allow_fail = 2; // allow 2 failures
  int fails = 0;
  while (num_trials--) {
    generate_stream({1024,0.002,0.5,0,"./sample.txt","./cumul_sample.txt"});
    ifstream in{"./sample.txt"};
    node_t n, m;
    in >> n >> m;
    Graph g{n};
    int type, a, b;
    while (m--) {
      in >> type >> a >> b;
      if (type == INSERT) {
        g.update({{a, b}, INSERT});
      } else g.update({{a, b}, DELETE});
    }

    g.set_verifier(std::make_unique<FileGraphVerifier>("./cumul_sample.txt"));
    try {
      g.connected_components();
    } catch (OutOfQueriesException& err) {
      fails++;
      if (fails > allow_fail) {
        printf("More than %i failures failing test\n", allow_fail);
        throw;
      }
    }
  }
}

TEST_P(GraphTest, TestCorrectnessOfReheating) {
  write_configuration(GetParam());
  int num_trials = 10;
  int allow_fail = 2; // allow 2 failures
  int fails = 0;
  while (num_trials--) {
    generate_stream({1024,0.002,0.5,0,"./sample.txt","./cumul_sample.txt"});
    ifstream in{"./sample.txt"};
    node_t n, m;
    in >> n >> m;
    Graph *g = new Graph (n);
    int type, a, b;
    printf("number of updates = %lu\n", m);
    while (m--) {
      in >> type >> a >> b;
      if (type == INSERT) g->update({{a, b}, INSERT});
      else g->update({{a, b}, DELETE});
    }
    g->write_binary("./out_temp.txt");
    g->set_verifier(std::make_unique<FileGraphVerifier>("./cumul_sample.txt"));
    vector<set<node_t>> g_res;
    try {
      g_res = g->connected_components();
    } catch (OutOfQueriesException& err) {
      fails++;
      continue;
      if (fails > allow_fail) {
        printf("More than %i failures failing test\n", allow_fail);
        throw;
      }
    }
    printf("number of CC = %lu\n", g_res.size());
    delete g; // delete g to avoid having multiple graphs open at once. Which is illegal.

    Graph reheated {"./out_temp.txt"};
    reheated.set_verifier(std::make_unique<FileGraphVerifier>("./cumul_sample.txt"));
    auto reheated_res = reheated.connected_components();
    printf("number of reheated CC = %lu\n", reheated_res.size());
    ASSERT_EQ(g_res.size(), reheated_res.size());
    for (unsigned i = 0; i < g_res.size(); ++i) {
      std::vector<node_t> symdif;
      std::set_symmetric_difference(g_res[i].begin(), g_res[i].end(),
          reheated_res[i].begin(), reheated_res[i].end(),
          std::back_inserter(symdif));
      ASSERT_EQ(0, symdif.size());
    }
  }
}
