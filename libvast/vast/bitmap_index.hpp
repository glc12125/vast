/******************************************************************************
 *                    _   _____   __________                                  *
 *                   | | / / _ | / __/_  __/     Visibility                   *
 *                   | |/ / __ |_\ \  / /          Across                     *
 *                   |___/_/ |_/___/ /_/       Space and Time                 *
 *                                                                            *
 * This file is part of VAST. It is subject to the license terms in the       *
 * LICENSE file found in the top-level directory of this distribution and at  *
 * http://vast.io/license. No part of VAST, including this file, may be       *
 * copied, modified, propagated, or distributed except according to the terms *
 * contained in the LICENSE file.                                             *
 ******************************************************************************/

#ifndef VAST_BITMAP_INDEX_HPP
#define VAST_BITMAP_INDEX_HPP

#include <type_traits>

#include "vast/base.hpp"
#include "vast/binner.hpp"
#include "vast/coder.hpp"
#include "vast/detail/order.hpp"

namespace vast {

class bitmap;

/// An associative array which maps arithmetic values to [bitmaps](@ref bitmap).
/// @tparam T The value type for append and lookup operation.
/// @tparam Base The base determining the value decomposition
/// @tparam Coder The encoding/decoding policy.
/// @tparam Binner The pre-processing policy to perform on values.
template <
  class T,
  class Coder = multi_level_coder<range_coder<bitmap>>,
  class Binner = identity_binner
>
class bitmap_index
  : detail::equality_comparable<bitmap_index<T, Coder, Binner>> {
  static_assert(!std::is_same<T, bool>{} || is_singleton_coder<Coder>{},
                "boolean bitmap index requires singleton coder");
public:
  using value_type = T;
  using coder_type = Coder;
  using binner_type = Binner;
  using bitmap_type = typename coder_type::bitmap_type;
  using size_type = typename coder_type::size_type;

  bitmap_index() = default;

  template <
    class... Ts,
    class = std::enable_if_t<std::is_constructible<coder_type, Ts...>{}>
  >
  explicit bitmap_index(Ts&&... xs)
    : coder_(std::forward<Ts>(xs)...) {
  }

  /// Appends a value to the bitmap index. For example, in the case of equality
  /// coding, this means appending 1 to the single bitmap for the given
  /// value and 0 to all other bitmaps.
  /// @param x The value to append.
  /// @post Skipped entries show up as 0s during decoding.
  void push_back(value_type x, size_type skip = 0) {
    append(x, 1, skip);
  }

  /// Appends one or more instances of value to the bitmap index.
  /// @param x The value to append.
  /// @param n The number of times to append *x*.
  /// @post Skipped entries show up as 0s during decoding.
  void append(value_type x, size_type n, size_type skip = 0) {
    coder_.encode(transform(binner_type::bin(x)), n, skip);
  }

  /// Appends the contents of another bitmap index to this one.
  /// @param other The other bitmap index.
  void append(const bitmap_index& other) {
    coder_.append(other.coder_);
  }

  /// Retrieves a bitmap of a given value with respect to a given operator.
  /// @param op The relational operator to use for looking up *x*.
  /// @param x The value to find the bitmap for.
  /// @returns The bitmap for all values *v* where *op(v,x)* is `true`.
  bitmap_type lookup(relational_operator op, value_type x) const {
    return coder_.decode(op, transform(binner_type::bin(x)));
  }

  /// Retrieves the bitmap index size.
  /// @returns The number of elements/rows contained in the bitmap index.
  size_type size() const {
    return coder_.size();
  }

  /// Checks whether the bitmap index is empty.
  /// @returns `true` *iff* the bitmap index has 0 entries.
  bool empty() const {
    return size() == 0;
  }

  /// Accesses the underlying coder of the bitmap index.
  /// @returns The coder of this bitmap index.
  const coder_type& coder() const {
    return coder_;
  }

  friend bool operator==(const bitmap_index& x, const bitmap_index& y) {
    return x.coder_ == y.coder_;
  }

  template <class Inspector>
  friend auto inspect(Inspector& f, bitmap_index& bmi) {
    return f(bmi.coder_);
  }

private:
  template <class U, class B>
  using is_shiftable =
    std::integral_constant<
      bool,
      (detail::is_precision_binner<B>{} || detail::is_decimal_binner<B>{})
        && std::is_floating_point<U>{}
    >;

  template <class U, class B = binner_type>
  static auto transform(U x)
  -> std::enable_if_t<is_shiftable<U, B>{}, detail::ordered_type<U>> {
    return detail::order(x) >> (52 - B::digits2);
  }

  template <class U, class B = binner_type>
  static auto transform(U x)
  -> std::enable_if_t<!is_shiftable<U, B>{}, detail::ordered_type<T>> {
    return detail::order(x);
  }

  coder_type coder_;
};

} // namespace vast

#endif
