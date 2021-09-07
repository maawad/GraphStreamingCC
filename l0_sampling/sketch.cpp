#include "../include/sketch.h"
#include <cassert>
#include <iostream>

long Sketch::bytes_size = sizeof(Sketch);
long Sketch::offset_c = 0;

Sketch::Sketch(vec_t n, long seed, double num_bucket_factor):
    seed(seed), n(n), num_bucket_factor(num_bucket_factor) {
  long len = num_entries(n, num_bucket_factor);

  // loop through all the bucket_a and bucket_c values and initialize them
  for (long i = 0; i < len; i++) {
    *bucket_a(i) = 0;
    *bucket_c(i) = 0;
  }
}

Sketch::Sketch(vec_t n, long seed, std::fstream &binary_in) : seed(seed), n(n) {
  binary_in.read((char*)&num_bucket_factor, sizeof(double));
  long len = num_entries(n, num_bucket_factor);
  binary_in.read((char*)(bucket_a(0)), len * sizeof(vec_t));
  binary_in.read((char*)(bucket_c(0)), len * sizeof(vec_hash_t));
}

Sketch::Sketch(const Sketch &old) : seed(old.seed), n(old.n),
num_bucket_factor(old.num_bucket_factor) {
  long len = num_entries(n, num_bucket_factor);

  // loop through all the bucket_a and bucket_c values and initialize them
  for (long i = 0; i < len; i++) {
    *bucket_a(i) = 0;
    *bucket_c(i) = 0;
  }
}

long Sketch::num_entries(vec_t n, double num_bucket_factor) {
  const unsigned num_buckets = bucket_gen(n, num_bucket_factor);
  const unsigned num_guesses = guess_gen(n);
  return num_buckets * num_guesses;
}

void Sketch::update(const vec_t& update_idx) {
  const unsigned num_buckets = bucket_gen(n, num_bucket_factor);
  const unsigned num_guesses = guess_gen(n);
  XXH64_hash_t update_hash = Bucket_Boruvka::index_hash(update_idx, seed);
  for (unsigned i = 0; i < num_buckets; ++i) {
    col_hash_t col_index_hash = Bucket_Boruvka::col_index_hash(i, update_idx, seed);
    for (unsigned j = 0; j < num_guesses; ++j) {
      unsigned bucket_id = i * num_guesses + j;
      if (Bucket_Boruvka::contains(col_index_hash, 1 << j)){
        Bucket_Boruvka::update(*bucket_a(bucket_id), *bucket_c(bucket_id), update_idx, update_hash);
      } else break;
    }
  }
}

void Sketch::batch_update(const std::vector<vec_t>& updates) {
  for (const auto& update_idx : updates) {
    update(update_idx);
  }
}

vec_t Sketch::query() {
  if (already_quered) {
    throw MultipleQueryException();
  }
  already_quered = true;
  bool all_buckets_zero = true;
  const unsigned num_buckets = bucket_gen(n, num_bucket_factor);
  const unsigned num_guesses = guess_gen(n);
  for (unsigned i = 0; i < num_buckets; ++i) {
    for (unsigned j = 0; j < num_guesses; ++j) {
      unsigned bucket_id = i * num_guesses + j;
      if (*bucket_a(bucket_id) != 0 || *bucket_c(bucket_id) != 0) {
        all_buckets_zero = false;
      }
      if (Bucket_Boruvka::is_good(*bucket_a(bucket_id), *bucket_c(bucket_id), n, i, 1 << j, seed)) {
        return *bucket_a(bucket_id);
      }
    }
  }
  if (all_buckets_zero) {
    throw AllBucketsZeroException();
  } else {
    throw NoGoodBucketException();
  }
}

Sketch &operator+= (Sketch &sketch1, const Sketch &sketch2) {
  assert (sketch1.n == sketch2.n);
  assert (sketch1.seed == sketch2.seed);
  assert (sketch1.num_bucket_factor == sketch2.num_bucket_factor);
  long len = Sketch::num_entries(sketch1.n, sketch1.num_bucket_factor);
  for (long i = 0; i < len; i++){
    *(sketch1.bucket_a(i)) ^= *(sketch2.bucket_a(i));
    *(sketch1.bucket_c(i)) ^= *(sketch2.bucket_c(i));
  }
  sketch1.already_quered = sketch1.already_quered || sketch2.already_quered;
  return sketch1;
}

bool operator== (const Sketch &sketch1, const Sketch &sketch2) {
  if (sketch1.n != sketch2.n || sketch1.seed != sketch2.seed ||
    sketch1.num_bucket_factor != sketch2.num_bucket_factor || 
    sketch1.already_quered == sketch2.already_quered) {
      return false;
    }

  long len = Sketch::num_entries(sketch1.n, sketch1.num_bucket_factor);
  for (long i = 0; i < len; i++) {
    if (*(sketch1.bucket_a(i)) != *(sketch2.bucket_a(i)) || 
      *(sketch1.bucket_c(i)) != *(sketch2.bucket_c(i))) {
      return false;
    }
      
  }

  return true;
}

std::ostream& operator<< (std::ostream &os, const Sketch &sketch) {
  const unsigned long long int num_buckets = bucket_gen(sketch.n, sketch.num_bucket_factor);
  const unsigned long long int num_guesses = guess_gen(sketch.n);
  for (unsigned i = 0; i < num_buckets; ++i) {
    for (unsigned j = 0; j < num_guesses; ++j) {
      unsigned bucket_id = i * num_guesses + j;
      for (unsigned k = 0; k < sketch.n; k++) {
        os << (Bucket_Boruvka::contains(Bucket_Boruvka::col_index_hash(i, k, sketch.seed), 1 << j) ? '1' : '0');
      }
      os << std::endl
         << "a:" << *sketch.bucket_a(bucket_id) << std::endl
         << "c:" << *sketch.bucket_c(bucket_id) << std::endl
         << (Bucket_Boruvka::is_good(*sketch.bucket_a(bucket_id), *sketch.bucket_c(bucket_id), sketch.n, i, 1 << j, sketch.seed) ? "good" : "bad") << std::endl;
    }
  }
  return os;
}

void Sketch::write_binary(std::fstream& binary_out) {
  binary_out.write((char*)&num_bucket_factor, sizeof(double));
  long len = num_entries(n, num_bucket_factor);
  binary_out.write((char*)bucket_a(0), len*sizeof(vec_t));
  binary_out.write((char*)bucket_c(0), len*sizeof(vec_hash_t));
}
