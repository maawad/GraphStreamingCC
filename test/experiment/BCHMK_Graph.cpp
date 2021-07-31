#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <thread>

#include "../../include/graph.h"

/*
 * Function which is run in a seperate thread and will query
 * the graph for the number of updates it has processed
 * the thread writes that information to a given file
 * @param total       the total number of edge updates
 * @param g           the graph object to query
 * @param start_time  the time that we started stream ingestion
 */
void query_insertions(uint64_t total, Graph *g, std::chrono::steady_clock::time_point start_time) {
  total = total * 2;                // we insert 2 edge updates per edge
  ofstream out{"runtime_stats.txt"}; // open the outfile

  printf("Insertions\n");
  printf("Progress:                    | 0%%\r"); fflush(stdout);
  std::chrono::steady_clock::time_point start = start_time;
  std::chrono::steady_clock::time_point prev  = start_time;
  uint64_t prev_updates = 0;
  int percent = 0;

  while(true) {
    sleep(5);
    uint64_t updates = g->num_updates;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double> total_diff = now - start;
    std::chrono::duration<double> cur_diff   = now - prev;
    prev = now; // reset start time to right after query

    // calculate the insertion rate and write to file
    uint64_t upd_delta = updates - prev_updates;
    // divide insertions per second by 2 because each edge is split into two updates
    // we care about edges per second not about stream updates
    int ins_per_sec = (((float)(upd_delta)) / cur_diff.count()) / 2;

    int amount = upd_delta / (total * .01);
    if (amount > 1) {
      percent += amount;
      out << percent << "% :\n";
      out << "Updates per second sinces last entry: " << ins_per_sec << "\n";
      out << "Time since last entry: " << (int) cur_diff.count() << "\n";
      out << "Total runtime so far: " << (int) total_diff.count() << "\n\n";

      prev_updates += upd_delta;
      printf("total updates = %lu", updates);
    }
    
    if (updates >= total)
      break;

    // display the progress
    int progress = updates / (total * .05);
    printf("Progress:%s%s", std::string(progress, '=').c_str(), std::string(20 - progress, ' ').c_str());
    printf("| %i%% -- %.2f per second\r", progress * 5, ins_per_sec); fflush(stdout);
  }
  printf("Progress:====================| Done      \n");
  out.close();
  return;
}


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
  std::thread querier(query_insertions, total, &g, start);

  while (m--) {
    in >> std::skipws >> u >> a >> b;

    if (u == INSERT)
      g.update({{a, b}, INSERT});
    else
      g.update({{a, b}, DELETE});
  }

  std::cout << "Starting CC" << std::endl;

  uint64_t num_CC = g.connected_components().size();

  querier.join();
  printf("querier done\n");
  long double time_taken = static_cast<std::chrono::duration<long double>>(g.end_time - start).count();
  long double CC_time = static_cast<std::chrono::duration<long double>>(g.CC_end_time - g.end_time).count();

  ofstream out{"runtime_stats.txt",  std::ofstream::out | std::ofstream::app}; // open the outfile
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
