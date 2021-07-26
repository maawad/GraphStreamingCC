#include <gtest/gtest.h>
#include <fstream>
#include "../../include/supernode.h"
#include "../../include/graph.h"

TEST(Benchmark, BCHMKpostProcSingleOnPaperclipGraph) {
  const std::string fname = __FILE__;
  size_t pos = fname.find_last_of("\\/");
  const std::string curr_dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);
  std::ifstream in{curr_dir + "/../res/paperclip.stream"};
  Node num_nodes;
  in >> num_nodes;
  long m;
  in >> m;
  Node a, b;
  Graph g{num_nodes};
  while (m--) {
    in >> a >> b;
    g.update({{a, b}, INSERT});
  }
  std::cout << "Number of CCs:" << g.connected_components().size() << std::endl;
}
