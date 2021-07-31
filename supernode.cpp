#include <stdexcept>
#include <cmath>
#include <boost/multiprecision/cpp_int.hpp>
#include <omp.h>
#include "include/supernode.h"
#include "include/util.h"
#include "include/graph_worker.h"

Supernode::Supernode(uint64_t n, long seed): sketches(log2(n)), idx(0), logn(log2
(n)), seed(seed), n(n) {
  // generate logn sketches for each supernode (read: node)
  for (int i=0;i<logn;++i)
    sketches[i] = new Sketch(n*n, seed++);
}

Supernode::~Supernode() {
  for (int i=0;i<logn;++i)
    delete sketches[i];
}

boost::optional<Edge> Supernode::sample() {
  if (idx == logn) throw OutOfQueriesException();
  vec_t query_idx;
  try {
    query_idx = sketches[idx++]->query();
  } catch (AllBucketsZeroException &e) {
    return {};
  }
  return inv_nondir_non_self_edge_pairing_fn(query_idx);
}

void Supernode::merge(Supernode &other) {
  idx = max(idx, other.idx);
  for (int i=idx;i<logn;++i) {
    (*sketches[i])+=(*other.sketches[i]);
  }
}

void Supernode::update(vec_t upd) {
  for (Sketch* s : sketches)
    s->update(upd);
}

void Supernode::apply_delta_update(const Supernode* delta_node) {
  std::unique_lock<std::mutex> lk(node_mt);
  for (unsigned i = 0; i < sketches.size(); ++i) {
    *sketches[i] += *delta_node->sketches[i];
  }
  lk.unlock();
}

Supernode* Supernode::delta_supernode(uint64_t n, long seed,
                                     const vector<vec_t> &updates) {
  auto* delta_node = new Supernode(n, seed);
  /*
   * Consider fiddling with environment vars
   * OMP_DYNAMIC: whether the OS is allowed to dynamically change the number
   * of threads employed for each parallel section
   * OMP_NUM_THREADS (or set_omp_num_threads): how many threads to spin up for
   * each parallel section. the default is (probably) one per CPU core
   * available, but we may want to set it lower if logn is a nice multiple of
   * a lower number.
   *
   * We may want to use omp option schedule(dynamic) or schedule(guided) if
   * there are very many more iterations of loop than threads. Dynamic
   * scheduling is good if loop iterations are expected to take very much
   * different amounts of time. Refer to
   * http://www.inf.ufsc.br/~bosco.sobral/ensino/ine5645/OpenMP_Dynamic_Scheduling.pdf
   * for a detailed explanation.
   */
  /*
   * Current impl uses default threads and parallelism within batched_update.
   * Considered using spin-threads and parallelism within sketch::update, but
   * this was slow (at least on small graph inputs).
   */
#pragma omp parallel for num_threads(GraphWorker::get_group_size()) default(shared)
  for (auto & delta_sketch : delta_node->sketches) {
    delta_sketch->batch_update(updates);
  }
  return delta_node;
}

