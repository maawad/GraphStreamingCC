#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <chrono>
#include <sstream>

#include "../../include/graph.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Incorrect number of arguments. "
                 "Expected two but got " << argc-1 << std::endl;
    exit(EXIT_FAILURE);
  }

  // create the thread which will perform buffered IO for us
  ifstream in{argv[1]};

  Node num_nodes;
  in >> num_nodes;
  long m;
  in >> m;
  long total = m;
  Node a, b;
  uint8_t u;
  Graph g{num_nodes};

  auto start = std::chrono::steady_clock::now();
  std::tuple<uint32_t, uint32_t, bool> edge;
  while (m--) {
    in >> std::skipws >> u >> a >> b;

    if (u == INSERT)
      g.update({{a, b}, INSERT});
    else
      g.update({{a, b}, DELETE});
  }

  std::cout << "Starting CC" << std::endl;

  uint64_t num_CC = g.connected_components().size();
  long double time_taken = static_cast<std::chrono::duration<long double>>(g.end_time - start).count();
  long double CC_time = static_cast<std::chrono::duration<long double>>(g.CC_end_time - g.end_time).count();

  ofstream out{"runtime_stats.txt"}; // open the outfile
  std::cout << "Number of connected components is " << num_CC << std::endl;
  std::cout << "Writing runtime stats to runtime_stats.txt\n";

  std::chrono::duration<double> runtime  = g.end_time - start;

  // calculate the insertion rate and write to file
  // insertion rate measured in stream updates 
  // (not in the two sketch updates we process per stream update)
  float ins_per_sec = (((float)(total)) / runtime.count());
  out << "Procesing " <<total << " updates took " << time_taken << " seconds, " << ins_per_sec << " per second\n";

  out << "Connected Components algorithm took " << CC_time << " and found " << num_CC << " CC\n";
  out.close();
}
