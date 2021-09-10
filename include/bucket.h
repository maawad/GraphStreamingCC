#pragma once
#include <vector>
#include <xxhash.h>
#include "types.h"

namespace Bucket_Boruvka {
  /**
   * Hashes the column index and the update index together.
   * This is used as a parameter to Bucket::contains.
   * @param bucket_col Column index of the bucket.
   * @param update_idx Update index.
   * @param sketch_seed The seed of the Sketch this Bucket belongs to.
   * @return The hash of (bucket_col, update_idx) using sketch_seed as a seed.
   */
  inline static col_hash_t col_index_hash(const unsigned bucket_col, const vec_t& update_idx, const long sketch_seed);

  /**
   * Hashes the index.
   * This is used to as a parameter to Bucket::update
   * @param index Update index.
   * @param seed The seed of the Sketch this Bucket belongs to.
   * @return The hash of the update index, using the sketch seed as a seed.
   */
  inline static vec_hash_t index_hash(const vec_t& index, long seed);

  /**
   * Checks whether the hash associated with the Bucket hashes the index to 0.
   * @param col_index_hash The return value to Bucket::col_index_hash
   * @param guess_nonzero A power of 2, used as a modulus
   * @return true if the index is NOT hashed to zero mod guess_nonzero.
   */
  inline static bool contains(const col_hash_t& col_index_hash, const vec_t& guess_nonzero);

  /**
   * Checks whether a Bucket is good, assuming the Bucket contains all elements.
   * @param a The bucket's a value.
   * @param c The bucket's c value.
   * @param n Size of the vector being sketched.
   * @param sketch_seed The seed of the Sketch this Bucket belongs to.
   * @return true if this Bucket is good, else false.
   */
  inline static bool is_good(const vec_t& a, const vec_hash_t& c, const vec_t& n, const long& sketch_seed);

  /**
   * Checks whether a Bucket is good.
   * @param a The bucket's a value.
   * @param c The bucket's c value.
   * @param n Size of the vector being sketched.
   * @param bucket_col This Bucket's column index.
   * @param guess_nonzero The guess of nonzero elements in the vector being sketched.
   * @param sketch_seed The seed of the Sketch this Bucket belongs to.
   * @return true if this Bucket is good, else false.
   */
  inline static bool is_good(const vec_t& a, const vec_hash_t& c, const vec_t& n, const unsigned bucket_col, const vec_t& guess_nonzero, const long& sketch_seed);

  /**
   * Updates a Bucket with the given update index
   * @param a The bucket's a value. Modified by this function.
   * @param c The bucket's c value. Modified by this function.
   * @param update_idx The update index
   * @param update_hash The hash of the update index, generated with Bucket::index_hash.
   */
  inline static void update(vec_t& a, vec_hash_t& c, const vec_t& update_idx, const vec_hash_t& update_hash);
}

inline col_hash_t Bucket_Boruvka::col_index_hash(const unsigned bucket_col, const vec_t& update_idx, const long sketch_seed) {
  struct {
    unsigned bucket_col;
    vec_t update_idx;
  } __attribute__((packed)) buf = {bucket_col, update_idx};
  return col_hash(&buf, sizeof(buf), sketch_seed);
}

inline vec_hash_t Bucket_Boruvka::index_hash(const vec_t& index, long sketch_seed) {
  return vec_hash(&index, sizeof(index), sketch_seed);

}

inline bool Bucket_Boruvka::contains(const col_hash_t& col_index_hash, const vec_t& guess_nonzero) {
  return col_index_hash % guess_nonzero == 0;
}

inline bool Bucket_Boruvka::is_good(const vec_t& a, const vec_hash_t& c, const vec_t& n, const long& sketch_seed) {
  return a < n && c == index_hash(a, sketch_seed);
}

inline bool Bucket_Boruvka::is_good(const vec_t& a, const vec_hash_t& c, const vec_t& n, const unsigned bucket_col, const vec_t& guess_nonzero, const long& sketch_seed) {
  return a < n && c == index_hash(a, sketch_seed)
    && contains(col_index_hash(bucket_col, a, sketch_seed), guess_nonzero);
}

inline void Bucket_Boruvka::update(vec_t& a, vec_hash_t& c, const vec_t& update_idx, const vec_hash_t& update_hash) {
  a ^= update_idx;
  c ^= update_hash;
}
