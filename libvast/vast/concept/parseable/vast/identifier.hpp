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

#ifndef VAST_CONCEPT_PARSEABLE_VAST_IDENTIFIER_HPP
#define VAST_CONCEPT_PARSEABLE_VAST_IDENTIFIER_HPP

#include "vast/concept/parseable/core/choice.hpp"
#include "vast/concept/parseable/core/plus.hpp"
#include "vast/concept/parseable/core/operators.hpp"
#include "vast/concept/parseable/string/char.hpp"
#include "vast/concept/parseable/string/char_class.hpp"

namespace vast {
namespace parsers {

auto const identifier = +(alnum | chr{'_'});

} // namespace parsers
} // namespace vast

#endif
