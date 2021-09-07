#pragma once
#include <cmath>
#include <exception>
#include <fstream>
#include <vector>
#include <mutex>
#include "bucket.h"
#include "types.h"
#include "util.h"
#include <gtest/gtest_prod.h>

#define bucket_gen(x, c) double_to_ull((c)*(log2(x)+1))
#define guess_gen(x) double_to_ull(log2(x)+2)

/**
 * An implementation of a "sketch" as defined in the L0 algorithm.
 * Note a sketch may only be queried once. Attempting to query multiple times will
 * raise an error.
 */
class Sketch {
  // Seed used for hashing operations in this sketch.
  const long seed;
  // Length of the vector this is sketching.
  const vec_t n;
  // Factor for how many buckets there are in this sketch.
  double num_bucket_factor;

  // Flag to keep track if this sketch has already been queried.
  bool already_quered = false;

  // Buckets of this sketch. The buckets are stored in memory following the class.
  // Length is bucket_gen(n, num_bucket_factor) * guess_gen(n).
  // For buckets[i * guess_gen(n) + j], the bucket has a 1/2^j probability
  // of containing an index.
  inline vec_t* bucket_a(vec_t idx) const {
    return (vec_t*)((char *)this + sizeof(Sketch) + (idx * sizeof(vec_t)));
  }
  inline vec_hash_t* bucket_c(vec_t idx) const {
    return (vec_hash_t*)((char *)this + sizeof(Sketch) + offset_c + (idx * sizeof(vec_hash_t)));
  }

  FRIEND_TEST(SketchTestSuite, TestExceptions);
  FRIEND_TEST(EXPR_Parallelism, N10kU100k);

public:
  static long bytes_size; // size of a sketch and associated vectors
  // how far after bucket_a is the beginning of bucket_c
  static long offset_c;
  static long num_entries(vec_t n, double num_bucket_factor=.5);

  /**
   * Construct a sketch of a vector of size n
   * @param n Length of the vector to sketch.
   * @param seed Seed to use for hashing operations
   * @param num_bucket_factor Factor to scale the number of buckets in this sketch
   */
  Sketch(vec_t n, long seed, double num_bucket_factor = .5);

  /**
   * Utility constructor to deserialize from a binary input stream.
   * @param n
   * @param seed
   * @param binary_in
   */
  Sketch(vec_t n, long seed, std::fstream& binary_in);

  Sketch(const Sketch &old);

  /**
   * Update a sketch based on information about one of its indices.
   * @param update the point update.
   */
  void update(const vec_t& update_idx);

  /**
   * Update a sketch given a batch of updates.
   * @param updates A vector of updates
   */
  void batch_update(const std::vector<vec_t>& updates);

  /**
   * Function to query a sketch.
   * @return                        an index.
   * @throws MultipleQueryException if the sketch has already been queried.
   * @throws NoGoodBucketException  if there are no good buckets to choose an
   *                                index from.
   */
  vec_t query();

  /**
   * Operator to add a sketch to another one in-place. Guaranteed to be
   * thread-safe for the sketch being added to. It is up to the user to
   * handle concurrency of the other sketch.
   * @param sketch1 the one being added to.
   * @param sketch2 the one being added.
   * @return a reference to the combined sketch.
   */
  friend Sketch &operator+= (Sketch &sketch1, const Sketch &sketch2);
  friend bool operator== (const Sketch &sketch1, const Sketch &sketch2);
  friend std::ostream& operator<< (std::ostream &os, const Sketch &sketch);

  /**
   * Serialize the sketch to a binary output stream.
   * @param out the stream to write to.
   */
  void write_binary(std::fstream& binary_out);
};

class AllBucketsZeroException : public std::exception {
public:
  virtual const char* what() const throw() {
    return "All buckets zero";
  }
};

class MultipleQueryException : public std::exception {
public:
  virtual const char* what() const throw() {
    return "This sketch has already been sampled!";
  }
};

class NoGoodBucketException : public std::exception {
public:
  virtual const char* what() const throw() {
    return "Found no good bucket!";
  }
};

