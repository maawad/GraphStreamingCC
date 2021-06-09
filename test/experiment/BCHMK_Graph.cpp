#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <ctime>

#include "../../include/graph.h"

int main() {
  // to use top/awk tool
  int child_pid = fork();
  if (child_pid != 0) {
    std::cout << child_pid << std::endl;
    return 0;
  }

  const std::string fname = __FILE__;
  size_t pos = fname.find_last_of("\\/");
  const std::string curr_dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);
  
  // before running this experiment, copy the test file to
  // the 'current_test.stream' file. This is so tests can
  // be automated.
  ifstream in{curr_dir + "/../res/current_test.stream"};
  Node num_nodes;
  in >> num_nodes;
  long m;
  in >> m;
  long total = m;
  Node a, b;
  uint8_t u;
  Graph g{num_nodes};

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

  while (m--) {
    in >> std::skipws >> u >> a >> b;
    if (u == INSERT)
      g.update({{a, b}, INSERT});
    else
      g.update({{a,b}, DELETE});
  }
  uint64_t num_CC = g.connected_components().size();
  printf("Number of connected components is %lu\n", num_CC);

  ofstream out{"runtime_data.txt"}; // open the outfile
  std::chrono::duration<double> runtime = g.end_time - start;

  // calculate the insertion rate and write to file
  // insertion rate measured in stream updates 
  // (not in the two sketch updates we process per stream update)
  float ins_per_sec = (((float)(total)) / runtime.count()) / 2;
  out << (total / 2) << "\t" << ins_per_sec << "\n";
  out.close();
}
