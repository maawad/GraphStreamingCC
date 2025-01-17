#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>
#include <buffering_system.h>

// forward declarations
class Graph;
class Supernode;

class GraphWorker {
public:
  /**
   * start_workers creates the graph workers and sets them to query the
   * given buffering system.
   * @param _graph           the graph to update.
   * @param _bf              the buffering system to query.
   * @param _supernode_size  the size of a supernode so that we can allocate
   *                         space for a delta_node.
   */
  static void start_workers(Graph *_graph, BufferingSystem *_bf, long _supernode_size);
  static void stop_workers();    // shutdown and delete GraphWorkers
  static void pause_workers();   // pause the GraphWorkers before CC
  static void unpause_workers(); // unpause the GraphWorkers to resume updates


  /**
   * Returns whether the current thread is paused.
   */
  bool get_thr_paused() {return thr_paused;}

  // manage configuration
  // configuration should be set before calling start_workers
  static int get_num_groups() {return num_groups;} // return the number of GraphWorkers
  static int get_group_size() {return group_size;} // return the number of threads in each worker
  static void set_config(int g, int s) { num_groups = g; group_size = s; }
private:
  /**
   * Create a GraphWorker object by setting metadata and spinning up a thread.
   * @param _id     the id of the new GraphWorker.
   * @param _graph  the graph which this GraphWorker will be updating.
   * @param _bf     the database data will be extracted from.
   */
  GraphWorker(int _id, Graph *_graph, BufferingSystem *_bf);
  ~GraphWorker();

  /**
   * This function is used by a new thread to capture the GraphWorker object
   * and begin running do_work.
   * @param obj the memory where we will store the GraphWorker obj.
   */
  static void *start_worker(void *obj) {
    ((GraphWorker *)obj)->do_work();
    return nullptr;
  }

  void do_work(); // function which runs the GraphWorker process
  int id;
  Graph *graph;
  BufferingSystem *bf;
  std::thread thr;
  bool thr_paused; // indicates if this individual thread is paused

  // thread status and status management
  static bool shutdown;
  static bool paused;
  static std::condition_variable pause_condition;
  static std::mutex pause_lock;

  // configuration
  static int num_groups;
  static int group_size;
  static long supernode_size;

  // list of all GraphWorkers
  static GraphWorker **workers;

  // the supernode object this GraphWorker will use for generating deltas
  Supernode *delta_node;
};
