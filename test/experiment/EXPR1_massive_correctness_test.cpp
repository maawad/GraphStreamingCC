#include <iostream>
#include "../../include/graph.h"
#include "../util/graph_gen.h"
#include <gtest/gtest.h>

TEST(ExperimentalSuite, TestCorrectnessOnMassiveGeneratedGraph) {
  const std::string fname = __FILE__;
  size_t pos = fname.find_last_of("\\/");
  const std::string curr_dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);
  ifstream in{curr_dir + "/../res/multiples_stream_10007.txt"};
  Node num_nodes;
  in >> num_nodes;
  long m;
  in >> m;
  Node a, b;
  int d;
  Graph g{num_nodes};
  while (m--) {
    in >>d >> a >> b;
    if (d == INSERT) {
      g.update({{a, b}, INSERT});
    } else {
      g.update({{a,b}, DELETE});
    }
  }
  g.set_cum_in(curr_dir + "/../res/multiples_graph_10007.txt");
  std::cout << "EXPR1 " << g.connected_components().size() << std::endl;
}
