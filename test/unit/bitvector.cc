#include "framework/unit.h"
#include "vast/bitvector.h"
#include "vast/util/print.h"

using namespace vast;

SUITE("bitvector")

TEST("to_string")
{
  bitvector a;
  bitvector b{10};
  bitvector c{78, true};

  CHECK(to_string(a) == "");
  CHECK(to_string(b) == "0000000000");
  CHECK(to_string(c), std::string(78, '1'));
}

TEST("basic operations")
{
  bitvector x;
  x.push_back(true);
  x.push_back(false);
  x.push_back(true);

  CHECK(x[0]);
  CHECK(! x[1]);
  CHECK(x[2]);

  CHECK(x.size() == 3);
  CHECK(x.blocks() == 1);

  x.append(0xf00f, 16);
  CHECK(x[3]);
  CHECK(x[18]);
  x.append(0xf0, 8);

  CHECK(x.blocks() == 1);
  CHECK(x.size() == 3 + 16 + 8);

  x.append(0);
  x.append(0xff, 8);
  CHECK(x.blocks() == 2);
  CHECK(x.size() == 3 + 16 + 8 + bitvector::block_width + 8);
}

TEST("bitwise operations")
{
  bitvector a{6};
  CHECK(a.size() == 6);
  CHECK(a.blocks() == 1);

  a.flip(3);
  CHECK(to_string(a) == "001000");
  CHECK(to_string(a << 1) == "010000");
  CHECK(to_string(a << 2) == "100000");
  CHECK(to_string(a << 3) == "000000");
  CHECK(to_string(a >> 1) == "000100");
  CHECK(to_string(a >> 2) == "000010");
  CHECK(to_string(a >> 3) == "000001");
  CHECK(to_string(a >> 4) == "000000");

  bitvector b{a};
  b[5] = b[1] = 1;
  CHECK(to_string(b) == "101010");
  CHECK(to_string(~b) == "010101");

  CHECK(to_string(a | ~b) == "011101");
  CHECK(to_string((~a << 2) & b) == to_string(a));

  CHECK(b.count() == 3);

  CHECK(to_string(b, false) == "010101");
}

TEST("backward search")
{
  bitvector x;
  x.append(0xffff);
  x.append(0x30abffff7000ffff);

  auto i = x.find_last();
  CHECK(i == 125);
  i = x.find_prev(i);
  CHECK(i == 124);
  i = x.find_prev(i);
  CHECK(i == 119);
  CHECK(x.find_prev(63) == 15);

  bitvector y;
  y.append(0xf0ffffffffffff0f);
  CHECK(y.find_last() == 63);
  CHECK(y.find_prev(59) == 55);
}

TEST("iteration")
{
  bitvector x;
  x.append(0x30abffff7000ffff);

  std::string str;
  std::transform(
      bitvector::const_bit_iterator::begin(x),
      bitvector::const_bit_iterator::end(x),
      std::back_inserter(str),
      [](bitvector::const_reference bit) { return bit ? '1' : '0'; });

  CHECK(to_string(x, false) == str);

  std::string rts;
  std::transform(
      bitvector::const_bit_iterator::rbegin(x),
      bitvector::const_bit_iterator::rend(x),
      std::back_inserter(rts),
      [](bitvector::const_reference bit) { return bit ? '1' : '0'; });

  std::reverse(str.begin(), str.end());
  CHECK(str == rts);

  std::string ones;
  std::transform(
      bitvector::const_ones_iterator::begin(x),
      bitvector::const_ones_iterator::end(x),
      std::back_inserter(ones),
      [](bitvector::const_reference bit) { return bit ? '1' : '0'; });

  CHECK(ones == "111111111111111111111111111111111111111111");

  auto i = bitvector::const_ones_iterator::rbegin(x);
  CHECK(i.base().position() == 61);
  ++i;
  CHECK(i.base().position() == 60);
  ++i;
  CHECK(i.base().position() == 55);
  while (i != bitvector::const_ones_iterator::rend(x))
    ++i;
  CHECK(i.base().position() == 0);

  auto j = bitvector::ones_iterator::rbegin(x);
  CHECK(j.base().position() == 61);
  *j.base() = false;
  ++j;
  *j.base() = false;
  j = bitvector::ones_iterator::rbegin(x);
  CHECK(j.base().position() == 55);
}
