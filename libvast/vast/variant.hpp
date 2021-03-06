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

// Copyright (C) 2013 Jarryd Beck
//
// (adapted by Matthias Vallentin for C++17 conformance).
//
// Distributed under the Boost Software License, Version 1.0
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
//   The copyright notices in the Software and this entire statement, including
//   the above license grant, this restriction and the following disclaimer,
//   must be included in all copies of the Software, in whole or in part, and
//   all derivative works of the Software, unless such copies or derivative
//   works are solely in the form of machine-executable object code generated by
//   a source language processor.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
//   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
//   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
//   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//   DEALINGS IN THE SOFTWARE.

#ifndef VAST_VARIANT_HPP
#define VAST_VARIANT_HPP

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>

#include <caf/detail/scope_guard.hpp>

#include "vast/config.hpp"

#include "vast/detail/assert.hpp"
#include "vast/detail/operators.hpp"
#include "vast/detail/type_traits.hpp"

namespace vast::detail {

template <class Visitor>
class delayed_visitor {
public:
  delayed_visitor(Visitor v) : visitor(std::move(v)) {
  }

  template <class... Visitables>
  decltype(auto) operator()(Visitables&&... vs) {
    return apply_visitor(visitor, std::forward<Visitables>(vs)...);
  }

private:
  Visitor visitor;
};

template <class Visitor>
class delayed_visitor_wrapper {
public:
  delayed_visitor_wrapper(Visitor& visitor) : visitor(visitor) {
  }

  template <class... Visitables>
  decltype(auto) operator()(Visitables&&... vs) {
    return apply_visitor(visitor, std::forward<Visitables>(vs)...);
  }

private:
  Visitor& visitor;
};

template <class Visitor, class Visitable>
class binary_visitor {
public:
  binary_visitor(Visitor& arg_visitor, Visitable& arg_visitable)
    : visitor(arg_visitor), visitable(arg_visitable) {
  }

  template <class... Ts>
  decltype(auto) operator()(Ts&&... xs) {
    return visitable.template apply(visitor, std::forward<Ts>(xs)...);
  }

private:
  Visitor& visitor;
  Visitable& visitable;
};

} // namespace vast::detail

namespace vast {

/// A variant class modeled after C++17's variant.
/// @tparam Ts the types the variant should assume.
template <class... Ts>
class variant : detail::totally_ordered<variant<Ts...>> {
public:
  using types = std::tuple<Ts...>;
  using first_type = std::tuple_element_t<0, types>;

  /// Constructs a variant from a type index.
  /// @param index The index of the type to default-construct.
  /// @pre `index < sizeof...(Ts)`
  static variant make(size_t index) {
    return variant{factory{}, index};
  }

  /// Default-construct a variant with the first type.
  variant() noexcept(std::is_nothrow_default_constructible<first_type>::value) {
    construct(first_type{});
    index_ = 0;
  }

   /// Destruct variant by invoking destructor of the active instance.
  ~variant() noexcept {
    destruct();
  }

  template <class T, class = detail::disable_if_same_or_derived_t<variant, T>>
  variant(T&& x) {
    // A compile error here means that T is not unambiguously convertible to
    // any of the variant types.
    initializer<0, Ts...>::initialize(*this, std::forward<T>(x));
  }

  variant(const variant& other)
  noexcept((std::is_nothrow_constructible_v<Ts> && ...)) {
    other.apply(copy_constructor{*this});
    index_ = other.index_;
  }

  variant(variant&& other)
  noexcept((std::is_nothrow_move_constructible_v<Ts> && ...)) {
    other.apply(move_constructor{*this});
    index_ = other.index_;
  }

  variant& operator=(const variant& rhs) {
    rhs.apply(assigner{*this, rhs.index_});
    index_ = rhs.index_;
    return *this;
  }

  variant& operator=(variant&& rhs)
  noexcept((std::is_nothrow_move_assignable_v<Ts> && ...)) {
    rhs.apply(move_assigner{*this, rhs.index_});
    index_ = rhs.index_;
    return *this;
  }

  /// @returns the index of the active variant type.
  size_t index() const {
    return index_;
  }

  template <class Visitor, class... Args>
  decltype(auto) apply(Visitor&& visitor, Args&&... args) {
    return visit_impl(index_, storage_, std::forward<Visitor>(visitor),
                      std::forward<Args>(args)...);
  }

  template <class Visitor, class... Args>
  decltype(auto) apply(Visitor&& visitor, Args&&... args) const {
    return visit_impl(index_, storage_, std::forward<Visitor>(visitor),
                      std::forward<Args>(args)...);
  }

private:
  struct factory {};
  variant(factory, size_t index) : index_{index} {
    VAST_ASSERT(index_ < sizeof...(Ts));
    apply(default_constructor{*this});
  }

  std::aligned_union_t<0, Ts...> storage_;
  size_t index_;

  struct default_constructor {
    default_constructor(variant& self) : self_(self) {
    }

    template <class T>
    void operator()(const T&) const
    noexcept(std::is_nothrow_default_constructible<T>{}) {
      self_.construct(T());
    }

  private:
    variant& self_;
  };

  struct copy_constructor {
    copy_constructor(variant& self) : self(self) {
    }

    template <class T>
    void operator()(const T& x) const
    noexcept(std::is_nothrow_copy_constructible<T>{}) {
      self.construct(x);
    }

    variant& self;
  };

  struct move_constructor {
    move_constructor(variant& self) : self(self) {
    }

    template <class T>
    void operator()(T& rhs) const
    noexcept(std::is_nothrow_move_constructible<T>{}) {
      self.construct(std::move(rhs));
    }

    variant& self;
  };

  struct assigner {
    template <class Rhs>
    void operator()(const Rhs& rhs) const
    noexcept(std::is_nothrow_copy_assignable<Rhs>{} &&
             std::is_nothrow_destructible<Rhs>{} &&
             std::is_nothrow_move_constructible<Rhs>{}) {
      if (self.index_ == rhs_index) {
        auto x = reinterpret_cast<Rhs*>(&self.storage_);
        *x = rhs;
      } else {
        Rhs tmp(rhs);
        self.destruct();
        self.construct(std::move(tmp));
      }
    }

    variant& self;
    size_t rhs_index;
  };

  template <class T>
  struct container_uses_default_allocator {
    static const bool value = false;
  };

  template <class CharT, class Traits>
  struct container_uses_default_allocator<std::basic_string<CharT, Traits>> {
    static const bool value = true;
  };

  template <class T, class Compare>
  struct container_uses_default_allocator<std::set<T, Compare>> {
    static const bool value = true;
  };

  template <class Key, class T, class Compare>
  struct container_uses_default_allocator<std::map<Key, T, Compare>> {
    static const bool value = true;
  };

  struct move_assigner {
    move_assigner(variant& self, size_t rhs_index)
      : self(self), rhs_index(rhs_index) {
    }

    template <class Rhs>
    void operator()(Rhs& rhs) const
    noexcept(std::is_nothrow_destructible<Rhs>{} &&
             std::is_nothrow_move_constructible<Rhs>{}) {
      using rhs_type = typename std::remove_const<Rhs>::type;
      if (self.index_ == rhs_index) {
        auto x = reinterpret_cast<rhs_type*>(&self.storage_);
        *x = std::move(rhs);
      } else {
        self.destruct();
        self.construct(std::move(rhs));
      }
    }

    variant& self;
    size_t rhs_index;
  };

  template <size_t TT, class... Tail>
  struct initializer;

  template <size_t TT, class T, class... Tail>
  struct initializer<TT, T, Tail...> : public initializer<TT + 1, Tail...> {
    using base = initializer<TT + 1, Tail...>;
    using base::initialize;

    static void initialize(variant& v, T&& x) {
      v.construct(std::move(x));
      v.index_ = TT;
    }

    static void initialize(variant& v, const T& x) {
      v.construct(x);
      v.index_ = TT;
    }
  };

  template <size_t TT>
  struct initializer<TT> {
    void initialize(); // this should never match
  };

  template <class T, class Storage>
  using const_type =
    typename std::conditional<
      std::is_const<std::remove_reference_t<Storage>>::value, T const, T
    >::type;

  template <class T, class Storage, class Visitor, class... Args>
  static decltype(auto) invoke(Storage&& storage, Visitor&& visitor,
                               Args&&... args) {
    auto x = reinterpret_cast<const_type<T, Storage>*>(&storage);
    return visitor(*x, args...);
  }

  template <class Storage, class Visitor, class... Args>
  static decltype(auto) visit_impl(size_t idx, Storage&& storage,
                                   Visitor&& visitor, Args&&... args) {
    using result_type = decltype(visitor(std::declval<first_type&>(), args...));
    using fn = result_type (*)(Storage&&, Visitor&&, Args&&...);
    static constexpr fn callers[sizeof...(Ts)]
      = {&invoke<Ts, Storage, Visitor, Args...>...};
    VAST_ASSERT(idx >= 0 && idx < sizeof...(Ts));
    return (*callers[idx])(std::forward<Storage>(storage),
                           std::forward<Visitor>(visitor),
                           std::forward<Args>(args)...);
  }

 // FIXME: GCC complains in variant_inspect_helper::inspect below that
 // construct is private to variant. But shouldn't it be irrelevant because
 // all structs defined inside a class are automatically befriended?
#ifdef VAST_GCC
public:
#endif
  template <class T>
  void construct(T&& x) {
    using type = std::remove_reference_t<T>;
    new (&storage_) type(std::forward<T>(x));
  }
#ifdef VAST_GCC
private:
#endif

  struct dtor {
    template <class T>
    void operator()(T& x) const noexcept {
      static_assert(std::is_nothrow_destructible<T>{},
                    "T must not throw in destructor");
      x.~T();
    }
  };

  void destruct() noexcept {
    apply(dtor{});
  }

  struct equals {
    equals(const variant& self) : self(self) {
    }

    template <class Rhs>
    bool operator()(const Rhs& rhs) const {
      auto x = reinterpret_cast<const Rhs*>(&self.storage_);
      return *x == rhs;
    }

    const variant& self;
  };

  struct less_than {
    less_than(const variant& self) : self(self) {
    }

    template <class Rhs>
    bool operator()(const Rhs& rhs) const {
      auto x = reinterpret_cast<const Rhs*>(&self.storage_);
      return *x < rhs;
    }

    const variant& self;
  };

  friend bool operator==(const variant& x, const variant& y) {
    return x.index_ == y.index_ && y.apply(equals{x});
  }

  friend bool operator<(const variant& x, const variant& y) {
    if (x.index_ == y.index_)
      return y.apply(less_than{x});
    else
      return x.index_ < y.index_;
  }

  template <class Inspector>
  friend std::enable_if_t<
    Inspector::reads_state,
    typename Inspector::result_type
  >
  inspect(Inspector& f, variant& v) {
    return apply_visitor([&](auto& x) { return f(v.index_, x); }, v);
  }

  struct variant_inspect_helper {
    template <class Inspector>
    friend auto inspect(Inspector& f, variant_inspect_helper& helper) {
      auto dispatcher = [&](auto& x) {
        std::decay_t<decltype(x)> value;
        auto guard = caf::detail::make_scope_guard([&] {
          // See note on construct() above.
          helper.self.construct(std::move(value));
        });
        return f(value);
      };
      return apply_visitor(dispatcher, helper.self);
    }

    variant& self;
  };

  template <class Inspector>
  friend std::enable_if_t<
    Inspector::writes_state,
    typename Inspector::result_type
  >
  inspect(Inspector& f, variant& v) {
    auto& index = v.index_;
    variant_inspect_helper helper{v};
    return f(index, helper);
  }
};

// Facilitate construction from types that take variadic template parameters,
// e.g., std::tuple.

template <class T>
struct make_variant_from;

template <template <class...> class T, class... Ts>
struct make_variant_from<T<Ts...>> {
  using type = variant<Ts...>;
};

namespace detail {

template <class Visitor>
delayed_visitor<Visitor> apply_visitor(Visitor&& visitor) {
  return delayed_visitor<Visitor>(std::move(visitor));
}

template <class Visitor>
delayed_visitor_wrapper<Visitor> apply_visitor(Visitor& visitor) {
  return delayed_visitor_wrapper<Visitor>(visitor);
}

// Can we please simplify this insane SFINAE stuff?
template <class Visitor, class Variant>
decltype(auto) apply_visitor(Visitor&& visitor, Variant&& v) {
  return v.template apply(std::forward<Visitor>(visitor));
}

template <class Visitor, class Variant, class... Variants>
decltype(auto) apply_visitor(Visitor&& visitor, Variant&& v, Variants&&... vs) {
  return apply_visitor(binary_visitor<Visitor, Variant>(visitor, v), vs...);
}

} // namespace detail

template <class T>
struct getter {
  T* operator()(T& val) const {
    return &val;
  }

  template <class U>
  T* operator()(const U&) const {
    return nullptr;
  }
};

// -- variant concept -------------------------------------------------------
//
// The *variant* concept comes in handy for types which contain a (recursive)
// variant but would like to be treated with variant interface externally.
// Such types benefit from uniform access of the variant aspect, namely
// visitation and selective type checking/extraction. To model the *variant*
// concept, a type `V` must provide the following free function to be found via
// ADL:
//
//    variant<Ts...>& expose(V& x)
//
// This function enables the following functions:
//
//    1) `decltype(auto) visit(Visitor, Vs&&... vs)`
//    2) `auto get_if<T>(V&& x)`
//    3) `auto get<T>(V&& x)`
//    4) `bool is<T>(V&& x)`
//
// (1)-(3) have the same semantics as the corresponding function from C++17's
// std::variant. (4) is an idiomatic shortcut for `get_if<T>(v) == nullptr`.
// Note that `variant<Ts...>` trivially models the variant concept.

template <class... Ts>
variant<Ts...>& expose(variant<Ts...>& v) {
  return v;
}

template <class T>
auto& expose(const T& v) {
  return expose(const_cast<T&>(v));
}

template <class Visitor, class... Vs>
decltype(auto) visit(Visitor&& v, Vs&&... vs) {
  return detail::apply_visitor(std::forward<Visitor>(v), expose(vs)...);
}

template <class T, class V>
auto get_if(V&& v) {
  return visit(getter<T>{}, v);
}

class bad_variant_access : public std::exception {
public:
  bad_variant_access() = default;

  const char* what() const noexcept override {
    return "bad variant access";
  }
};

template <class T, class V>
auto& get(V&& v) {
  if (auto ptr = get_if<T>(v))
    return *ptr;
  throw bad_variant_access{};
}

template <class T, class V>
bool is(V&& v) {
  return get_if<T>(v) != nullptr;
}

} // namespace vast

#endif // VAST_VARIANT_HPP
