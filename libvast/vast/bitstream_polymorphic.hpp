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

#ifndef VAST_BITSTREAM_POLYMORPHIC_HPP
#define VAST_BITSTREAM_POLYMORPHIC_HPP

#include "vast/bitstream.hpp"
#include "vast/die.hpp"
#include "vast/util/iterator.hpp"
#include "vast/util/meta.hpp"

namespace vast {

struct access;
class bitstream;

namespace detail {

class bitstream_concept {
  friend bitstream;

public:
  using size_type = bitvector::size_type;
  using block_type = bitvector::block_type;

private:
  class iterator_concept {
  public:
    iterator_concept() = default;
    virtual ~iterator_concept() = default;
    virtual std::unique_ptr<iterator_concept> copy() const = 0;

    virtual bool equals(const iterator_concept& other) const = 0;
    virtual void increment() = 0;
    virtual size_type dereference() const = 0;
  };

  /// A concrete model for a specific bitstream iterator.
  template <class Iterator>
  class iterator_model : public iterator_concept {
  public:
    iterator_model() = default;

    iterator_model(Iterator&& i) : iterator_{std::move(i)} {
    }

  private:
    virtual std::unique_ptr<iterator_concept> copy() const final {
      return std::make_unique<iterator_model>(*this);
    }

    virtual bool equals(const iterator_concept& other) const final {
      auto x = dynamic_cast<const iterator_model*>(&other);
      if (x == nullptr)
        die("bad iterator cast");
      return iterator_ == x->iterator_;
    }

    virtual void increment() final {
      ++iterator_;
    }

    virtual size_type dereference() const final {
      return *iterator_;
    }

    Iterator iterator_;
  };

public:
  using const_iterator = class iterator
    : public util::iterator_facade<
        iterator, size_type, std::forward_iterator_tag, size_type
      > {
  public:
    iterator() = default;

    template <
      class Iterator,
      class = util::disable_if_same_or_derived_t<iterator, Iterator>
    >
    iterator(Iterator&& i)
      : concept_{new iterator_model<std::decay_t<Iterator>>{
          std::forward<Iterator>(i)}} {
    }

    iterator(const iterator& other);
    iterator(iterator&& other);
    iterator& operator=(const iterator& other);
    iterator& operator=(iterator&& other);

  private:
    friend util::iterator_access;

    bool equals(const iterator& other) const;
    void increment();
    size_type dereference() const;

    std::unique_ptr<iterator_concept> concept_;
  };

  virtual ~bitstream_concept() = default;
  virtual std::unique_ptr<bitstream_concept> copy() const = 0;

  // Interface as required by bitstream_base<T>.
  virtual bool equals(const bitstream_concept& other) const = 0;
  virtual void bitwise_not() = 0;
  virtual void bitwise_and(const bitstream_concept& other) = 0;
  virtual void bitwise_or(const bitstream_concept& other) = 0;
  virtual void bitwise_xor(const bitstream_concept& other) = 0;
  virtual void bitwise_subtract(const bitstream_concept& other) = 0;
  virtual void append_impl(const bitstream_concept& other) = 0;
  virtual void append_impl(size_type n, bool bit) = 0;
  virtual void append_block_impl(block_type block, size_type bits) = 0;
  virtual void push_back_impl(bool bit) = 0;
  virtual void trim_impl() = 0;
  virtual void clear_impl() noexcept = 0;
  virtual bool at(size_type i) const = 0;
  virtual size_type size_impl() const = 0;
  virtual size_type count_impl() const = 0;
  virtual bool empty_impl() const = 0;
  virtual const_iterator begin_impl() const = 0;
  virtual const_iterator end_impl() const = 0;
  virtual bool back_impl() const = 0;
  virtual size_type find_first_impl() const = 0;
  virtual size_type find_next_impl(size_type i) const = 0;
  virtual size_type find_last_impl() const = 0;
  virtual size_type find_prev_impl(size_type i) const = 0;
  virtual const bitvector& bits_impl() const = 0;

protected:
  bitstream_concept() = default;
};

/// A concrete bitstream.
template <class Bitstream>
class bitstream_model : public bitstream_concept,
                        util::equality_comparable<bitstream_model<Bitstream>> {
  friend access;

  const Bitstream& cast(const bitstream_concept& c) const {
    auto x = dynamic_cast<const bitstream_model*>(&c);
    if (x == nullptr)
      die("bad bitstream cast");
    return x->bitstream_;
  }

  Bitstream& cast(bitstream_concept& c) {
    auto x = dynamic_cast<bitstream_model*>(&c);
    if (x == nullptr)
      die("bad bitstream cast");
    return x->bitstream_;
  }

  friend bool operator==(const bitstream_model& x, const bitstream_model& y) {
    return x.bitstream_ == y.bitstream_;
  }

  template <class Inspector>
  friend auto inspect(Inspector& f, bitstream_model& bsm) {
    return f(bsm.bitstream_);
  }

public:
  bitstream_model() = default;

  bitstream_model(Bitstream bs) : bitstream_(std::move(bs)) {
  }

  virtual std::unique_ptr<bitstream_concept> copy() const final {
    return std::make_unique<bitstream_model>(*this);
  }

  virtual bool equals(const bitstream_concept& other) const final {
    return bitstream_.equals(cast(other));
  }

  virtual void bitwise_not() final {
    bitstream_.bitwise_not();
  }

  virtual void bitwise_and(const bitstream_concept& other) final {
    bitstream_.bitwise_and(cast(other));
  }

  virtual void bitwise_or(const bitstream_concept& other) final {
    bitstream_.bitwise_or(cast(other));
  }

  virtual void bitwise_xor(const bitstream_concept& other) final {
    bitstream_.bitwise_xor(cast(other));
  }

  virtual void bitwise_subtract(const bitstream_concept& other) final {
    bitstream_.bitwise_subtract(cast(other));
  }

  virtual void append_impl(const bitstream_concept& other) final {
    bitstream_.append_impl(cast(other));
  }

  virtual void append_impl(size_type n, bool bit) final {
    bitstream_.append_impl(n, bit);
  }

  virtual void append_block_impl(block_type block, size_type bits) final {
    bitstream_.append_block_impl(block, bits);
  }

  virtual void push_back_impl(bool bit) final {
    bitstream_.push_back_impl(bit);
  }

  virtual void trim_impl() final {
    bitstream_.trim_impl();
  }

  virtual void clear_impl() noexcept final {
    bitstream_.clear_impl();
  }

  virtual bool at(size_type i) const final {
    return bitstream_.at(i);
  }

  virtual size_type size_impl() const final {
    return bitstream_.size_impl();
  }

  virtual size_type count_impl() const final {
    return bitstream_.count_impl();
  }

  virtual bool empty_impl() const final {
    return bitstream_.empty_impl();
  }

  virtual const_iterator begin_impl() const final {
    return const_iterator{bitstream_.begin_impl()};
  }

  virtual const_iterator end_impl() const final {
    return const_iterator{bitstream_.end_impl()};
  }

  virtual bool back_impl() const final {
    return bitstream_.back_impl();
  }

  virtual size_type find_first_impl() const final {
    return bitstream_.find_first_impl();
  }

  virtual size_type find_next_impl(size_type i) const final {
    return bitstream_.find_next_impl(i);
  }

  virtual size_type find_last_impl() const final {
    return bitstream_.find_last_impl();
  }

  virtual size_type find_prev_impl(size_type i) const final {
    return bitstream_.find_prev_impl(i);
  }

  virtual const bitvector& bits_impl() const final {
    return bitstream_.bits_impl();
  }

private:
  Bitstream bitstream_;
};

} // namespace detail

/// A polymorphic bitstream with value semantics.
class bitstream : public bitstream_base<bitstream>,
                  util::equality_comparable<bitstream> {
  friend access;
  friend bool operator==(const bitstream& x, const bitstream& y);

public:
  using iterator = detail::bitstream_concept::iterator;
  using const_iterator = detail::bitstream_concept::const_iterator;

  bitstream() = default;
  bitstream(const bitstream& other);
  bitstream(bitstream&& other);

  template <
    class Bitstream,
    class = util::disable_if_same_or_derived_t<bitstream, Bitstream>
  >
  explicit bitstream(Bitstream&& bs)
    : concept_{new detail::bitstream_model<std::decay_t<Bitstream>>{
        std::forward<Bitstream>(bs)}} {
  }

  bitstream& operator=(const bitstream& other);
  bitstream& operator=(bitstream&& other);

  explicit operator bool() const;

  template <class Inspector>
  friend auto inspect(Inspector& f, bitstream& bs) {
    return f(bs.concept_);
  }

private:
  friend bitstream_base<bitstream>;

  bool equals(const bitstream& other) const;
  void bitwise_not();
  void bitwise_and(const bitstream& other);
  void bitwise_or(const bitstream& other);
  void bitwise_xor(const bitstream& other);
  void bitwise_subtract(const bitstream& other);
  void append_impl(const bitstream& other);
  void append_impl(size_type n, bool bit);
  void append_block_impl(block_type block, size_type bits);
  void push_back_impl(bool bit);
  void trim_impl();
  void clear_impl() noexcept;
  bool at(size_type i) const;
  size_type size_impl() const;
  size_type count_impl() const;
  bool empty_impl() const;
  const_iterator begin_impl() const;
  const_iterator end_impl() const;
  bool back_impl() const;
  size_type find_first_impl() const;
  size_type find_next_impl(size_type i) const;
  size_type find_last_impl() const;
  size_type find_prev_impl(size_type i) const;
  const bitvector& bits_impl() const;

  std::unique_ptr<detail::bitstream_concept> concept_;
};

} // namespace vast

#endif
