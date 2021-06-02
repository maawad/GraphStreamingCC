#include "../include/sketch.h"
#include <chrono>
#include <gtest/gtest.h>
#include "util/stats.h"
#include "util/testing_vector.h"

TEST(SketchTestSuite, TestExceptions) {
  Sketch sketch1(10,rand(), 1);
  ASSERT_THROW(sketch1.query(), AllBucketsZeroException);
  ASSERT_THROW(sketch1.query(), MultipleQueryException);

  /**
   * Find a vector that makes no good buckets
   */
  Sketch sketch2(100, 0);
  std::vector<bool> vec_idx(sketch2.n, true);
  unsigned long long num_buckets = bucket_gen(sketch2.n, 1);
  unsigned long long num_guesses = guess_gen(sketch2.n);
  for (unsigned long long i = 0; i < num_buckets; ++i) {
    for (unsigned long long j = 0; j < num_guesses;) {
      uint64_t index = 0;
      for (uint64_t k = 0; k < sketch2.n; ++k) {
        if (vec_idx[k] && Bucket_Boruvka::contains(Bucket_Boruvka::col_index_hash(i, k, sketch2.seed), 1 << j)) {
          if (index == 0) {
            index = k + 1;
          } else {
            index = 0;
            break;
          }
        }
      }
      if (index) {
        vec_idx[index - 1] = false;
        i = j = 0;
      } else {
        ++j;
      }
    }
  }
  for (uint64_t i = 0; i < sketch2.n; ++i) {
    if (vec_idx[i]) {
      sketch2.update(static_cast<vec_t>(i));
    }
  }
  ASSERT_THROW(sketch2.query(), NoGoodBucketException);
}

TEST(SketchTestSuite, GIVENonlyIndexZeroUpdatedTHENitWorks) {
  // GIVEN only the index 0 is updated
  srand(time(NULL));
  int vec_size = 1000;
  Sketch sketch(vec_size, rand());
  sketch.update(0);
  sketch.update(0);
  sketch.update(0);

  // THEN it works
  vec_t res = sketch.query();
  ASSERT_EQ(res, 0) << "Expected: 0" << std::endl << "Actual: " << res;
}

/**
 * Make sure sketch sampling works
 */
void test_sketch_sample(unsigned long num_sketches, unsigned long vec_size,
    unsigned long num_updates, double num_bucket_factor, double alpha) {
  srand(time(NULL));
  std::chrono::duration<long double> runtime(0);
  unsigned long all_bucket_failures = 0;
  unsigned long sample_incorrect_failures = 0;
  for (unsigned long i = 0; i < num_sketches; i++) {
    Testing_Vector test_vec = Testing_Vector(vec_size, num_updates);
    Sketch sketch = Sketch(vec_size, rand(), num_bucket_factor);
    auto start_time = std::chrono::steady_clock::now();
    for (unsigned long j = 0; j < num_updates; j++){
      sketch.update(test_vec.get_update(j));
    }
    runtime += std::chrono::steady_clock::now() - start_time;
    try {
      vec_t res_idx = sketch.query();
      //Multiple queries shouldn't happen, but if we do get here fail test
      ASSERT_LT(res_idx, vec_size) << "Sampled index out of bounds";
      if (!test_vec.get_entry(res_idx)) {
        //Undetected sample error
        sample_incorrect_failures++;
      }
    } catch (AllBucketsZeroException& e) {
      //All buckets being 0 implies that the whole vector should be 0
      bool vec_zero = true;
      for (unsigned long j = 0; vec_zero && j < vec_size; j++) {
        if (test_vec.get_entry(j)) {
          vec_zero = false;
        }
      }
      if (!vec_zero) {
        sample_incorrect_failures++;
      }
    } catch (NoGoodBucketException& e) {
      //No good bucket
      all_bucket_failures++;
    } catch (MultipleQueryException& e) {
      //Multiple queries shouldn't happen, but if we do get here fail test
      FAIL() << e.what();
    }
  }
  std::cout << "Updating " << num_sketches << " sketches of length "
      << vec_size << " vectors with " << num_updates << " updates took "
      << runtime.count() << std::endl;
  double x0 = std::pow(vec_size, -num_bucket_factor);
  double area_left = binomcdf(all_bucket_failures, num_sketches, x0);
  double area_right = all_bucket_failures == 0 ? 1 : 1 - binomcdf(all_bucket_failures - 1, num_sketches, x0);
  std::cout << "Expect " << x0 << ", got " << static_cast<double>(all_bucket_failures) / num_sketches
      << ". p-left " << area_left << ", p-right " << area_right << std::endl;
  if (area_right < alpha) {
    ADD_FAILURE() << "failure probability significantly greater than 1/n^c";
  } else if (area_left < alpha) {
    std::cout << "significantly less" << std::endl;
  }
}

TEST(SketchTestSuite, TestSketchSample) {
  srand (time(NULL));
  test_sketch_sample(10000, 100, 100, 1, .001);
  test_sketch_sample(1000, 1000, 1000, 1, .001);
  test_sketch_sample(1000, 10000, 10000, 1, .001);
}

/**
 * Make sure sketch addition works
 */
void test_sketch_addition(unsigned long num_sketches, unsigned long vec_size,
    unsigned long num_updates, double num_bucket_factor, double alpha) {
  srand (time(NULL));
  unsigned long all_bucket_failures = 0;
  unsigned long sample_incorrect_failures = 0;
  for (unsigned long i = 0; i < num_sketches; i++){
    const long seed = rand();
    Sketch sketch1 = Sketch(vec_size, seed, num_bucket_factor);
    Sketch sketch2 = Sketch(vec_size, seed, num_bucket_factor);
    Testing_Vector test_vec1 = Testing_Vector(vec_size, num_updates);
    Testing_Vector test_vec2 = Testing_Vector(vec_size, num_updates);

    for (unsigned long j = 0; j < num_updates; j++){
      sketch1.update(test_vec1.get_update(j));
      sketch2.update(test_vec2.get_update(j));
    }
    sketch1 += sketch2;
    try {
      vec_t res_idx = sketch1.query();
      ASSERT_LT(res_idx, vec_size) << "Sampled index out of bounds";
      if (test_vec1.get_entry(res_idx) == test_vec2.get_entry(res_idx)) {
        sample_incorrect_failures++;
      }
    } catch (AllBucketsZeroException& e) {
      //All buckets being 0 implies that the whole vector should be 0
      bool vec_zero = true;
      for (unsigned long j = 0; vec_zero && j < vec_size; j++) {
        if (test_vec1.get_entry(j) != test_vec2.get_entry(j)) {
          vec_zero = false;
        }
      }
      if (!vec_zero) {
        sample_incorrect_failures++;
      }
    } catch (NoGoodBucketException& e) {
      //No good bucket
      all_bucket_failures++;
    } catch (MultipleQueryException& e) {
      //Multiple queries shouldn't happen, but if we do get here fail test
      FAIL() << e.what();
    }
  }
  double x0 = std::pow(vec_size, -num_bucket_factor);
  double area_left = binomcdf(all_bucket_failures, num_sketches, x0);
  double area_right = all_bucket_failures == 0 ? 1 : 1 - binomcdf(all_bucket_failures - 1, num_sketches, x0);
  std::cout << "Expect " << x0 << ", got " << static_cast<double>(all_bucket_failures) / num_sketches
      << ". p-left " << area_left << ", p-right " << area_right << std::endl;
  if (area_right < alpha) {
    ADD_FAILURE() << "failure probability significantly greater than 1/n^c";
  } else if (area_left < alpha) {
    std::cout << "significantly less" << std::endl;
  }
}

TEST(SketchTestSuite, TestSketchAddition){
  test_sketch_addition(10000, 100, 100, 1, 0.001);
  test_sketch_addition(1000, 1000, 1000, 1, 0.001);
  test_sketch_addition(1000, 10000, 10000, 1, 0.001);
}

/**
 * Large sketch test
 */
void test_sketch_large(unsigned long num_sketches, unsigned long vec_size,
    unsigned long num_updates, double num_bucket_factor, double alpha) {
  srand(time(NULL));
  // Round num_updates to odd number so we don't need to deal with all_bucket_zero
  num_updates |= 1;
  std::chrono::duration<long double> runtime(0);
  unsigned long all_bucket_failures = 0;
  unsigned long sample_incorrect_failures = 0;
  for (unsigned long i = 0; i < num_sketches; i++) {
    Sketch sketch = Sketch(vec_size, rand());
    //Keep seed for replaying update stream later
    unsigned long seed = rand();
    srand(seed);
    auto start_time = std::chrono::steady_clock::now();
    for (unsigned long j = 0; j < num_updates; j++){
      sketch.update(static_cast<vec_t>(rand() % vec_size));
    }
    runtime += std::chrono::steady_clock::now() - start_time;
    try {
      vec_t res_idx = sketch.query();
      //Multiple queries shouldn't happen, but if we do get here fail test
      ASSERT_LT(res_idx, vec_size) << "Sampled index out of bounds";
      //Replay update stream, keep track of the sampled index
      srand(seed);
      bool actual_delta = false;
      for (unsigned long j = 0; j < num_updates; j++){
        vec_t update_idx = static_cast<vec_t>(rand() % vec_size);
        if (update_idx == res_idx) {
          actual_delta = !actual_delta;
        }
      }
      //Undetected sample error, not likely to happen for large vectors
      if (!actual_delta) {
        sample_incorrect_failures++;
      }
    } catch (AllBucketsZeroException& e) {
      // All buckets being 0 implies that the whole vector should be 0,
      // doesn't happen for odd number of updates
      FAIL() << "AllBucketsZeroException:" << e.what();
    } catch (NoGoodBucketException& e) {
      //No good bucket
      all_bucket_failures++;
    } catch (MultipleQueryException& e) {
      //Multiple queries shouldn't happen, but if we do get here fail test
      FAIL() << "MultipleQueryException:" << e.what();
    }
  }

  std::cout << "Updating " << num_sketches << " sketches of length "
      << vec_size << " vectors with " << num_updates << " updates took "
      << runtime.count() << std::endl;
  double x0 = std::pow(vec_size, -num_bucket_factor);
  double area_left = binomcdf(all_bucket_failures, num_sketches, x0);
  double area_right = all_bucket_failures == 0 ? 1 : 1 - binomcdf(all_bucket_failures - 1, num_sketches, x0);
  std::cout << "Expect " << x0 << ", got " << static_cast<double>(all_bucket_failures) / num_sketches
      << ". p-left " << area_left << ", p-right " << area_right << std::endl;
  if (area_right < alpha) {
    ADD_FAILURE() << "failure probability significantly greater than 1/n^c";
  } else if (area_left < alpha) {
    std::cout << "significantly less" << std::endl;
  }
}

TEST(SketchTestSuite, TestSketchLarge) {
  constexpr uint64_t upper_bound =
#ifdef USE_NATIVE_F
      1e9
#else
      1e18
#endif
  ;
  for (uint64_t i = 1000; i <= upper_bound; i *= 10) {
    test_sketch_large(1, i, 1000000, 1, 1);
  }
}

TEST(SketchTestSuite, TestBatchUpdate) {
  unsigned long vec_size = 1000000000, num_updates = 10000;
  srand(time(NULL));
  std::vector<vec_t> updates(num_updates);
  for (unsigned long i = 0; i < num_updates; i++) {
    updates[i] = static_cast<vec_t>(rand() % vec_size);
  }
  auto sketch_seed = rand();
  Sketch sketch(vec_size, sketch_seed);
  Sketch sketch_batch(vec_size, sketch_seed);
  auto start_time = std::chrono::steady_clock::now();
  for (const vec_t& update : updates) {
    sketch.update(update);
  }
  std::cout << "One by one updates took " << static_cast<std::chrono::duration<long double>>(std::chrono::steady_clock::now() - start_time).count() << std::endl;
  start_time = std::chrono::steady_clock::now();
  sketch_batch.batch_update(updates);
  std::cout << "Batched updates took " << static_cast<std::chrono::duration<long double>>(std::chrono::steady_clock::now() - start_time).count() << std::endl;

  ASSERT_EQ(sketch, sketch_batch);
}

TEST(SketchTestSuite, DISABLED_TestFailureRate) {
  constexpr uint64_t upper_bound =
#ifdef USE_NATIVE_F
      1e9
#else
      1e18
#endif
  ;
  for (uint64_t i = 10; i <= upper_bound; i *= 10) {
    test_sketch_large(1000, i, 1000000, 1, 1);
    test_sketch_large(1000, i, 1000000, .5, 1);
    test_sketch_large(1000, i, 1000000, .25, 1);
  }
}

void test_continuous(unsigned long vec_size, unsigned long updates_per_sample,
    unsigned long samples, double num_bucket_factor) {
  srand(time(NULL));
  unsigned long failures = 0;
  std::vector<bool> actual_vec(vec_size);
  Sketch sketch(vec_size, rand(), num_bucket_factor);
  for (unsigned long i = 0; i < samples; i++) {
    for (unsigned long j = 0; j < updates_per_sample; j++) {
      vec_t update = static_cast<vec_t>(rand() % vec_size);
      actual_vec[update] = !actual_vec[update];
      sketch.update(update);
    }
    try {
      if (!actual_vec[sketch.query()]) failures++;
    } catch (const AllBucketsZeroException& e) {
      if (std::any_of(actual_vec.cbegin(), actual_vec.cend(), [](bool x){return x;})) {
        failures++;
      }
    } catch (const NoGoodBucketException& e) {
      failures++;
    }
    sketch.already_quered = false;
  }
  std::cout << vec_size << ' ' << samples << ' ' << updates_per_sample << ' ' << failures << std::endl;
}

TEST(SketchTestSuite, DISABLED_TestContinuous) {
  for (unsigned long i = 10; i <= 1e10; i *= 10) {
    test_continuous(i, 1000, 1000000, 1);
    test_continuous(i, 1000, 1000000, .5);
    test_continuous(i, 1000, 1000000, .25);
  }
}
