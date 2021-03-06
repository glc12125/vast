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

#ifndef VAST_CONCEPT_PARSEABLE_CORE_AND_HPP
#define VAST_CONCEPT_PARSEABLE_CORE_AND_HPP

#include "vast/concept/parseable/core/parser.hpp"

namespace vast {

// The AND parser does not consume its input and serves as basic look-ahead.
template <class Parser>
class and_parser : public parser<and_parser<Parser>> {
public:
  using attribute = unused_type;

  explicit and_parser(Parser p) : parser_{std::move(p)} {
  }

  template <class Iterator, class Attribute>
  bool parse(Iterator& f, const Iterator& l, Attribute&) const {
    auto i = f; // Do not consume input.
    return parser_(i, l, unused);
  }

private:
  Parser parser_;
};

} // namespace vast

#endif
