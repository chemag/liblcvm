#pragma once

#include <cstdint>
#include <limits>

namespace liblcvm_detail {

// Explicitly retain the low 32 bits in the existing signed storage type.
inline int32_t wrap_timestamp(int64_t value) {
  const uint32_t bits = static_cast<uint32_t>(value);
  const int64_t signed_value =
      bits <= static_cast<uint32_t>(INT32_MAX)
          ? static_cast<int64_t>(bits)
          : static_cast<int64_t>(bits) - (INT64_C(1) << 32);
  return static_cast<int32_t>(signed_value);
}

inline bool add_timestamp(int64_t value, int64_t offset, int64_t& result) {
  constexpr int64_t low = std::numeric_limits<int64_t>::min();
  constexpr int64_t high = std::numeric_limits<int64_t>::max();
  if ((offset > 0 && value > high - offset) ||
      (offset < 0 && value < low - offset))
    return false;
  result = value + offset;
  return true;
}

// Unwrapped intermediates preserve the wrap count and direction for diff(a,b).
inline bool diff_timestamp(int64_t a, int64_t b, int64_t& result) {
  constexpr int64_t low = std::numeric_limits<int64_t>::min();
  constexpr int64_t high = std::numeric_limits<int64_t>::max();
  if ((a > 0 && b < low + a) || (a < 0 && b > high + a))
    return false;
  result = b - a;
  return true;
}

}  // namespace liblcvm_detail
