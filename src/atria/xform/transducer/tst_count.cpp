//
// Copyright (C) 2014, 2015 Ableton AG, Berlin. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <atria/xform/concepts.hpp>
#include <atria/xform/into_vector.hpp>
#include <atria/xform/transducer/count.hpp>
#include <atria/xform/transducer/take.hpp>
#include <atria/prelude/comp.hpp>
#include <atria/testing/gtest.hpp>

namespace atria {
namespace xform {

TEST(count, count)
{
  using tup = std::tuple<int, std::size_t>;
  auto v = std::vector<int> { 13, 42, 5 };

  auto res = into_vector(count(), v);
  EXPECT_EQ(res, (std::vector<tup> {{
        tup(13, 0u),
        tup(42, 1u),
        tup(5, 2u)
  }}));
}

TEST(count, generator)
{
  auto res = into_vector(comp(count('a'), take(3)));
  EXPECT_EQ(res, (std::vector<char> {{ 'a', 'b', 'c' }}));
}

TEST(count, stepped)
{
  auto res = into_vector(comp(count(1.0, 0.5), take(3)));
  EXPECT_EQ(res, (std::vector<double> {{ 1.0, 1.5, 2.0 }}));
}

} // namespace xform
} // namespace atria
