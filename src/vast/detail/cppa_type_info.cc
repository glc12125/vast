#include "vast/detail/cppa_type_info.h"

#include <ze/chunk.h>
#include <ze/event.h>
#include <ze/uuid.h>
#include <cppa/announce.hpp>
#include "vast/detail/cppa_archive.h"
#include "vast/schema.h"
#include "vast/segment.h"
#include "vast/to_string.h"
#include "vast/expression.h"

namespace vast {
namespace detail {

void cppa_announce_types()
{
  using namespace cppa;
  announce(typeid(ze::uuid), new uuid_type_info);
  announce<std::vector<ze::uuid>>();
  announce(typeid(ze::event), new event_type_info);
  announce<std::vector<ze::event>>();
  announce(typeid(ze::chunk<ze::event>), new event_chunk_type_info);

  announce(typeid(expression), new expression_type_info);
  announce(typeid(segment), new segment_type_info);
  announce(typeid(schema), new schema_type_info);
}

void uuid_type_info::serialize(void const* ptr, cppa::serializer* sink) const
{
    auto uuid = reinterpret_cast<ze::uuid const*>(ptr);
    cppa_oarchive oa(sink, name());
    oa << *uuid;
}

void uuid_type_info::deserialize(void* ptr, cppa::deserializer* source) const
{
    std::string cname = source->seek_object();
    if (cname != name())
        throw std::logic_error("wrong type name found");

    auto uuid = reinterpret_cast<ze::uuid*>(ptr);
    cppa_iarchive ia(source, cname);
    ia >> *uuid;
}

void event_type_info::serialize(void const* ptr, cppa::serializer* sink) const
{
    auto e = reinterpret_cast<ze::event const*>(ptr);
    cppa_oarchive oa(sink, name());
    oa << *e;
}

void event_type_info::deserialize(void* ptr, cppa::deserializer* source) const
{
    std::string cname = source->seek_object();
    if (cname != name())
        throw std::logic_error("wrong type name found");

    auto e = reinterpret_cast<ze::event*>(ptr);
    cppa_iarchive ia(source, cname);
    ia >> *e;
}

void event_chunk_type_info::serialize(void const* ptr, cppa::serializer* sink) const
{
    auto chk = reinterpret_cast<ze::chunk<ze::event> const*>(ptr);
    cppa_oarchive oa(sink, name());
    oa << *chk;
}

void event_chunk_type_info::deserialize(void* ptr, cppa::deserializer* source) const
{
    std::string cname = source->seek_object();
    if (cname != name())
        throw std::logic_error("wrong type name found");

    auto chk = reinterpret_cast<ze::chunk<ze::event>*>(ptr);
    cppa_iarchive ia(source, cname);
    ia >> *chk;
}

void segment_type_info::serialize(void const* ptr, cppa::serializer* sink) const
{
    auto s = reinterpret_cast<segment const*>(ptr);
    cppa_oarchive oa(sink, name());
    oa << *s;
}

void segment_type_info::deserialize(void* ptr, cppa::deserializer* source) const
{
    std::string cname = source->seek_object();
    if (cname != name())
        throw std::logic_error("wrong type name found");

    auto s = reinterpret_cast<segment*>(ptr);
    cppa_iarchive ia(source, cname);
    ia >> *s;
}

void expression_type_info::serialize(void const* ptr, cppa::serializer* sink) const
{
    auto expr = reinterpret_cast<expression const*>(ptr);
    cppa_oarchive oa(sink, name());
    oa << *expr;
}

void expression_type_info::deserialize(void* ptr, cppa::deserializer* source) const
{
    std::string cname = source->seek_object();
    if (cname != name())
        throw std::logic_error("wrong type name found");

    auto expr = reinterpret_cast<expression*>(ptr);
    cppa_iarchive ia(source, cname);
    ia >> *expr;
}

void schema_type_info::serialize(void const* ptr, cppa::serializer* sink) const
{
    auto sch = reinterpret_cast<schema const*>(ptr);
    cppa_oarchive oa(sink, name());
    oa << *sch;
}

void schema_type_info::deserialize(void* ptr, cppa::deserializer* source) const
{
    std::string cname = source->seek_object();
    if (cname != name())
        throw std::logic_error("wrong type name found");

    auto sch = reinterpret_cast<schema*>(ptr);
    cppa_iarchive ia(source, cname);
    ia >> *sch;
}

} // namespace detail
} // namespace vast
