#pragma once
#include <cmath>
#include <vector>
#include <exception>
#include "update.h"
#include "bucket_native.h"
#include "util.h"
#include <gtest/gtest_prod.h>

using namespace std;

#define bucket_gen(x) double_to_ull(log2(x)+1)
#define guess_gen(x) double_to_ull(log2(x)+2)

/**
 * A native implementation of a "sketch" as defined in the L0 algorithm.
 * Note a sketch may only be queried once. Attempting to query multiple times will
 * raise an error.
 */
class Sketch {
  const long seed;
  const uint64_t n;
  std::vector<Bucket> buckets;
  const long long int large_prime;
  bool already_quered = false;

  FRIEND_TEST(SketchTestSuite, TestExceptions);

  //Initialize a sketch of a vector of size n
public:
  Sketch(uint64_t n, long seed);

  /**
   * Update a sketch based on information about one of its indices.
   * @param update the point update.
   */
  void update(Update update);

  /**
   * Function to query a sketch.
   * @return                        an index in the form of an Update.
   * @throws MultipleQueryException if the sketch has already been queried.
   * @throws NoGoodBucketException  if there are no good buckets to choose an
   *                                index from.
   */
  Update query();

  friend Sketch operator+ (const Sketch &sketch1, const Sketch &sketch2);
  friend Sketch &operator+= (Sketch &sketch1, const Sketch &sketch2);
  friend Sketch operator* (const Sketch &sketch1, long scaling_factor );
  friend std::ostream& operator<< (std::ostream &os, const Sketch &sketch);
};

class AllBucketsZeroException : public exception {
public:
  virtual const char* what() const throw() {
    return "All buckets zero";
  }
};

class MultipleQueryException : public exception {
public:
  virtual const char* what() const throw() {
    return "This sketch has already been sampled!";
  }
};

class NoGoodBucketException : public exception {
public:
  virtual const char* what() const throw() {
    return "Found no good bucket!";
  }
};