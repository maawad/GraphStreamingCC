#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <ctime>

#include "../../include/graph.h"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Incorrect number of arguments. "
                 "Expected two but got " << argc-1 << std::endl;
    exit(EXIT_FAILURE);
  }

  std::cout << "Input stream: " << argv[1] << std::endl;
  ifstream in{argv[1]};

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

  std::cout << "Starting CC" << std::endl;

  uint64_t num_CC = g.connected_components().size();
  printf("Number of connected components is %lu\n", num_CC);

  ofstream out{argv[2]}; // open the outfile
  std::cout << "Writing runtime stats to " << argv[2] << std::endl;
  std::chrono::duration<double> runtime = g.end_time - start;

  // calculate the insertion rate and write to file
  // insertion rate measured in stream updates 
  // (not in the two sketch updates we process per stream update)
  float ins_per_sec = (((float)(total)) / runtime.count()) / 2;
  out << (total / 2) << "\t" << ins_per_sec << "\n";
  out.close();
}
