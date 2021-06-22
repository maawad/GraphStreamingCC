#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <chrono>
#include <sstream>

#include "../../include/graph.h"
#include "../../include/BufferedEdgeInput.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Incorrect number of arguments. "
                 "Expected two but got " << argc-1 << std::endl;
    exit(EXIT_FAILURE);
  }

  // create the thread which will perform buffered IO for us
  std::cout << "Input stream: " << argv[1] << std::endl;
  BufferedEdgeInput buffer(argv[1], 1000000);

  Node num_nodes = buffer.num_nodes;
  long m = buffer.num_edges;

  long total = m;
  Graph g{num_nodes};

  std::cout << "num nodes = " << num_nodes << " num edges = " << m << std::endl;
 
  auto start = std::chrono::steady_clock::now();
  std::tuple<uint32_t, uint32_t, bool> edge;
  while (m--) {
    buffer.get_edge(edge);

    //std::cout << std::get<0>(edge) << ", " << std::get<1>(edge) << std::endl; 

    if (std::get<2>(edge) == INSERT)
      g.update({{std::get<0>(edge), std::get<1>(edge)}, INSERT});
    else
      g.update({{std::get<0>(edge), std::get<1>(edge)}, DELETE});
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
