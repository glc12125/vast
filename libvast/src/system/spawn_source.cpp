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

#include <caf/all.hpp>

#include <caf/detail/scope_guard.hpp>

#include "vast/config.hpp"

#include "vast/concept/parseable/to.hpp"
#include "vast/concept/parseable/vast/expression.hpp"
#include "vast/concept/parseable/vast/schema.hpp"
#include "vast/error.hpp"
#include "vast/expression.hpp"
#include "vast/query_options.hpp"

#include "vast/format/bgpdump.hpp"
#include "vast/format/mrt.hpp"
#include "vast/format/bro.hpp"
#ifdef VAST_HAVE_PCAP
#include "vast/format/pcap.hpp"
#endif
#include "vast/format/test.hpp"

#include "vast/system/atoms.hpp"
#include "vast/system/source.hpp"
#include "vast/system/spawn.hpp"
#include "vast/system/format_factory.hpp"

#include "vast/detail/make_io_stream.hpp"

using namespace std::string_literals;
using namespace caf;

namespace vast {
namespace system {

expected<actor> spawn_source(caf::stateful_actor<node_state>* self,
                             options& opts) {
  if (opts.params.empty())
    return make_error(ec::syntax_error, "missing format");
  auto& formats = self->state.formats;
  auto& format = opts.params.get_as<std::string>(0);
  auto source_args = opts.params.drop(1);
  // Parse format-independent parameters first.
  auto input = "-"s;
  std::string schema_file;
  auto r = source_args.extract_opts({
    {"read,r", "path to input where to read events from", input},
    {"schema,s", "path to alternate schema", schema_file},
    {"uds,d", "treat -r as listening UNIX domain socket"}
  });
  // Ensure that, upon leaving this function, we have updated the parameter
  // list such that it no longer contains the command line options that we have
  // used in this function
  auto grd = caf::detail::make_scope_guard([&] { opts.params = r.remainder; });
  // Parse format-specific parameters, if any.
  actor src;
  if (format == "pcap") {
    auto reader = formats.reader(format);
    if (reader)
      src = (*reader)(self, opts.params);  
  } else if (format == "bro" || format == "bgpdump" || format == "mrt") {
    auto in = detail::make_input_stream(input, r.opts.count("uds") > 0);
    if (!in)
      return in.error();
    if (format == "bro") {
      format::bro::reader reader{std::move(*in)};
      src = self->spawn(source<format::bro::reader>, std::move(reader));
    } else if (format == "bgpdump") {
      format::bgpdump::reader reader{std::move(*in)};
      src = self->spawn(source<format::bgpdump::reader>, std::move(reader));
    } else if (format == "mrt") {
      auto reader = formats.reader(format);
      if (reader)
        src = (*reader)(self, opts.params);  
    }
  } else if (format == "test") {
    auto seed = size_t{0};
    auto id = event_id{0};
    auto n = uint64_t{100};
    r = r.remainder.extract_opts({
      {"seed,s", "the PRNG seed", seed},
      {"events,n", "number of events to generate", n},
      {"id,i", "the base event ID", id}
    });
    if (!r.error.empty())
      return make_error(ec::syntax_error, r.error);
    format::test::reader reader{seed, n, id};
    src = self->spawn(source<format::test::reader>, std::move(reader));
    // Since the test source doesn't consume any data and only generates
    // events out of thin air, we use the input channel to specify the schema.
    schema_file = input;
  } else {
    return make_error(ec::syntax_error, "invalid format:", format);
  }
  // Supply an alternate schema, if requested.
  if (!schema_file.empty()) {
    auto str = load_contents(schema_file);
    if (!str)
      return str.error();
    auto sch = to<schema>(*str);
    if (!sch)
      return sch.error();
    // Send anonymously, since we can't process the reply here.
    anon_send(src, put_atom::value, std::move(*sch));
  }
  // Attempt to parse the remainder as an expression.
  if (!r.remainder.empty()) {
    auto str = r.remainder.get_as<std::string>(0);
    for (auto i = 1u; i < r.remainder.size(); ++i)
      str += ' ' + r.remainder.get_as<std::string>(i);
    auto expr = to<expression>(str);
    if (!expr)
      return expr.error();
    expr = normalize_and_validate(*expr);
    if (!expr)
      return expr.error();
    r.remainder = {};
    anon_send(src, std::move(*expr));
  }
  return src;
}

} // namespace system
} // namespace vast
