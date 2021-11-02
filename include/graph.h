#pragma once
#include <cstdlib>
#include <exception>
#include <set>
#include <fstream>
#include <atomic>  // REMOVE LATER

#include <buffering_system.h>
#include "supernode.h"

#ifdef VERIFY_SAMPLES_F
#include "test/graph_verifier.h"
#endif

using namespace std;

// forward declarations
class GraphWorker;

typedef pair<Edge, UpdateType> GraphUpdate;

/**
 * Undirected graph object with n nodes labelled 0 to n-1, no self-edges,
 * multiple edges, or weights.
 */
class Graph {
  uint64_t num_nodes;
  long seed;
  bool update_locked = false;
  // a set containing one "representative" from each supernode
  set<node_t>* representatives;
  Supernode** supernodes;
  // DSU representation of supernode relationship
  node_t* parent;
  node_t get_parent(node_t node);

  // Buffering system for batching updates
  BufferingSystem *bf;

  Supernode** backup_supernodes();
  void restore_supernodes(Supernode** supernodes);

  FRIEND_TEST(GraphTestSuite, TestCorrectnessOfReheating);
  static bool open_graph;
public:
  explicit Graph(uint64_t num_nodes);
  explicit Graph(const string &input_file);

  ~Graph();
  void update(GraphUpdate upd);

  /**
   * Update all the sketches in supernode, given a batch of updates.
   * @param src        The supernode where the edges originate.
   * @param edges      A vector of <destination, delta> pairs.
   * @param delta_loc  Memory location where we should initialize the delta
   *                   supernode.
   */
  void batch_update(uint64_t src, const std::vector<uint64_t>& edges, Supernode *delta_loc);

  /**
   * Main algorithm utilizing Boruvka and L_0 sampling.
   * @return a vector of the connected components in the graph.
   */
  vector<set<node_t>> connected_components();

  /**
   * Main algorithm utilizing Boruvka and L_0 sampling.
   * If cont is true, allow for additional updates when done.
   * @param cont
   * @return a vector of the connected components in the graph.
   */
  vector<set<node_t>> connected_components(bool cont);

  /**
   * Parallel version of Boruvka.
   * @return a vector of the connected components in the graph.
   */
  vector<set<node_t>> parallel_connected_components();

#ifdef VERIFY_SAMPLES_F
  std::unique_ptr<GraphVerifier> verifier;
  void set_verifier(std::unique_ptr<GraphVerifier> verifier) {
    this->verifier = std::move(verifier);
  }
#endif

  // temp to verify number of updates -- REMOVE later
  std::atomic<uint64_t> num_updates;

  /**
   * Generate a delta node for the purposes of updating a node sketch
   * (supernode).
   * @param node_n     the total number of nodes in the graph.
   * @param node_seed  the seed of the supernode in question.
   * @param src        the src id.
   * @param edges      a list of node ids to which src is connected.
   * @param delta_loc  the preallocated memory where the delta_node should be
   *                   placed. this allows memory to be reused by the same
   *                   calling thread.
   * @returns nothing (supernode delta is in delta_loc).
   */
  static void generate_delta_node(uint64_t node_n, long node_seed, uint64_t src,
                 const vector<uint64_t> &edges, Supernode *delta_loc);

  /**
   * Serialize the graph data to a binary file.
   * @param filename the name of the file to (over)write data to.
   */
  void write_binary(const string &filename);

  std::chrono::steady_clock::time_point end_time;
};

class UpdateLockedException : public exception {
  virtual const char* what() const throw() {
    return "The graph cannot be updated: Connected components algorithm has "
           "already started";
  }
};

class MultipleGraphsException : public exception {
  virtual const char * what() const throw() {
    return "Only one Graph may be open at one time. The other Graph must be deleted.";
  }
};

