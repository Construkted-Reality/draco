// Copyright 2024 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef DRACO_ATTRIBUTES_FLAT_DEDUP_TABLE_H_
#define DRACO_ATTRIBUTES_FLAT_DEDUP_TABLE_H_

#include <cstdint>
#include <utility>
#include <vector>

namespace draco {

// A flat, fixed-capacity, linear-probing hash table used to deduplicate
// three-component 32-bit attribute values (e.g. float positions or texture
// coordinates bit-copied to uint32_t).
//
// Compared to std::unordered_map, this table uses a single contiguous
// allocation with open addressing, avoiding the per-node allocation and
// pointer chasing of a node-based map. It is sized once from the maximum
// number of entries and never rehashes or grows; deletion is not supported.
//
// A slot is empty iff its value word equals kEmptyValue. The stored values are
// dense first-occurrence indices in [0, num_entries), which are always far
// below kEmptyValue, so no valid value collides with the empty sentinel.
class FlatDedupTableU32x3 {
 public:
  // Value-word sentinel marking an empty slot. Valid stored values are
  // first-occurrence indices and are never this large.
  static constexpr uint32_t kEmptyValue = 0xFFFFFFFFu;

  // |max_entries| is the maximum number of distinct keys that can be inserted.
  // The capacity is the smallest power of two that keeps the load factor below
  // ~0.77, so probe chains stay short.
  explicit FlatDedupTableU32x3(uint32_t max_entries) {
    const uint64_t min_capacity =
        static_cast<uint64_t>(max_entries) * 13u / 10u + 1u;
    uint32_t capacity = 8;
    while (static_cast<uint64_t>(capacity) < min_capacity) {
      capacity <<= 1;
    }
    mask_ = capacity - 1;
    // Four words per slot (k0, k1, k2, value), all initialized to the empty
    // sentinel. Only the value word is read by the empty test.
    slots_.assign(static_cast<size_t>(capacity) * 4u, kEmptyValue);
  }

  // Inserts the key (|k0|, |k1|, |k2|) with the given |value| if it is not
  // already present, mirroring std::unordered_map::insert():
  //   * first occurrence -> stores |value|, returns {value, true}
  //   * existing key     -> returns {stored_value, false}
  std::pair<uint32_t, bool> Insert(uint32_t k0, uint32_t k1, uint32_t k2,
                                   uint32_t value) {
    uint32_t index = Hash(k0, k1, k2) & mask_;
    for (;;) {
      uint32_t *const slot = &slots_[static_cast<size_t>(index) * 4u];
      if (slot[3] == kEmptyValue) {
        slot[0] = k0;
        slot[1] = k1;
        slot[2] = k2;
        slot[3] = value;
        return {value, true};
      }
      if (slot[0] == k0 && slot[1] == k1 && slot[2] == k2) {
        return {slot[3], false};
      }
      index = (index + 1) & mask_;
    }
  }

 private:
  // Finalizer-style mix over the 12-byte key (MurmurHash3 fmix64 constants).
  // Hash quality affects probe length only, never correctness.
  static inline uint32_t Hash(uint32_t a, uint32_t b, uint32_t c) {
    uint64_t h = 0x9E3779B97F4A7C15ull;
    h ^= static_cast<uint64_t>(a);
    h *= 0xFF51AFD7ED558CCDull;
    h ^= static_cast<uint64_t>(b) + 0x9E3779B97F4A7C15ull;
    h *= 0xC4CEB9FE1A85EC53ull;
    h ^= static_cast<uint64_t>(c);
    h *= 0xFF51AFD7ED558CCDull;
    h ^= h >> 33;
    return static_cast<uint32_t>(h);
  }

  uint32_t mask_;
  std::vector<uint32_t> slots_;
};

}  // namespace draco

#endif  // DRACO_ATTRIBUTES_FLAT_DEDUP_TABLE_H_
