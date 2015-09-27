#ifndef VAST_CONCEPT_PARSEABLE_VAST_PORT_H
#define VAST_CONCEPT_PARSEABLE_VAST_PORT_H

#include "vast/port.h"

#include "vast/concept/parseable/core/parser.h"
#include "vast/concept/parseable/numeric/integral.h"
#include "vast/concept/parseable/vast/address.h"

namespace vast {

template <>
struct access::parser<port> : vast::parser<access::parser<port>> {
  using attribute = port;

  template <typename Iterator>
  bool parse(Iterator& f, Iterator const& l, unused_type) const {
    using namespace parsers;
    auto p = u16 >> '/' >> ("?"_p | "tcp" | "udp" | "icmp");
    return p.parse(f, l, unused);
  }

  template <typename Iterator>
  bool parse(Iterator& f, Iterator const& l, port& a) const {
    using namespace parsers;
    static auto p
      =  u16
      >> '/'
      >> ( "?"_p ->* [] { return port::unknown; }
         | "tcp"_p ->* [] { return port::tcp; }
         | "udp"_p ->* [] { return port::udp; }
         | "icmp"_p ->* [] { return port::icmp; }
         )
      ;
    auto t = std::tie(a.number_, a.type_);
    return p.parse(f, l, t);
  }
};

template <>
struct parser_registry<port> {
  using type = access::parser<port>;
};

namespace parsers {

static auto const port = make_parser<vast::port>();

} // namespace parsers

} // namespace vast

#endif
