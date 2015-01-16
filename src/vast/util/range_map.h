#ifndef VAST_UTIL_INTERVAL_MAP_H
#define VAST_UTIL_INTERVAL_MAP_H

#include <cassert>
#include <map>
#include <tuple>
#include "vast/util/iterator.h"

namespace vast {
namespace util {

/// An associative data structure that maps half-open, *disjoint* intervals to
/// values.
template <typename Point, typename Value>
class range_map
{
  using map_type = std::map<Point, std::pair<Point, Value>>;
  using map_iterator = typename map_type::iterator;
  using map_const_iterator = typename map_type::const_iterator;

public:
  class const_iterator
    : public iterator_adaptor<
        const_iterator,
        map_const_iterator,
        std::bidirectional_iterator_tag,
        std::tuple<Point, Point, Value>,
        std::tuple<Point const&, Point const&, Value const&>
      >
  {
    using super = iterator_adaptor<
      const_iterator,
      map_const_iterator,
      std::bidirectional_iterator_tag,
      std::tuple<Point, Point, Value>,
      std::tuple<Point const&, Point const&, Value const&>
    >;

  public:
    using super::super;

  private:
    friend util::iterator_access;

    std::tuple<Point const&, Point const&, Value const&> dereference() const
    {
      return std::tie(this->base()->first,
                      this->base()->second.first,
                      this->base()->second.second);
    }
  };

  const_iterator begin() const
  {
    return const_iterator{map_.begin()};
  }

  const_iterator end() const
  {
    return const_iterator{map_.end()};
  }

  /// Associates a value with a right-open range.
  /// @param l The left endpoint of the interval.
  /// @param r The right endpoint of the interval.
  /// @param v The value r associated with *[l, r]*.
  /// @returns `true` on success.
  bool insert(Point l, Point r, Value v)
  {
    assert(l < r);
    auto lb = map_.lower_bound(l);
    if (find(l, lb) != map_.end())
      return false;
    if (lb == map_.end() || lb->first >= r)
      return map_.emplace(
          std::move(l), std::make_pair(std::move(r), std::move(v))).second;
    return false;
  }

  /// Removes a value given a point from a right-open range.
  /// @param p A point from a range that maps to a value.
  /// @returns `true` if the value associated with the interval containing *p*
  ///          has been successfully removed, and `false` if *p* does not map
  ///          to an existing value.
  bool erase(Point p)
  {
    auto i = find(p, map_.lower_bound(p));
    if (i == map_.end())
      return false;
    map_.erase(i);
    return true;
  }

  /// Retrieves the value for a given point.
  /// @param p The point to lookup.
  /// @returns A pointer to the value associated with the half-open interval
  ///          *[a,b)* if *a <= p < b* and `nullptr` otherwise.
  Value const* lookup(Point const& p) const
  {
    auto i = find(p, map_.lower_bound(p));
    return i != map_.end() ? &i->second.second : nullptr;
  }

  /// Retrieves value and interval for a given point.
  /// @param p The point to lookup.
  /// @returns A tuple with the first component holding a pointer to the value
  ///          associated with the half-open interval *[a,b)* if *a <= p < b*,
  ///          and `nullptr` otherwise. If the first component points to a
  ///          valid value, then the remaining two represent *[a,b)* and are
  ///          set to *[0,0)* otherwise.
  std::tuple<Value const*, Point, Point> find(Point const& p) const
  {
    auto i = find(p, map_.lower_bound(p));
    if (i == map_.end())
      return {nullptr, 0, 0};
    else
      return {&i->second.second, i->first, i->second.first};
  }

  /// Retrieves the size of the range map.
  /// @returns The number of entries in the map.
  size_t size() const
  {
    return map_.size();
  }

  /// Checks whether the range map is empty.
  /// @returns `true` iff the map is empty.
  bool empty() const
  {
    return map_.empty();
  }

private:
  map_const_iterator find(Point const& p, map_const_iterator lb) const
  {
    if ((lb != map_.end() && lb->first == p) ||
        (lb != map_.begin() && p < (--lb)->second.first))
      return lb;
    return map_.end();
  }

  map_type map_;
};

} // namespace util
} // namespace vast

#endif
