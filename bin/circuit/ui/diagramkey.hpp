#pragma once

#include <utility>

namespace key {
// make a const_iterator from an iterator for the same container
// to avoid mixing iterators with const_iterators warning
template<typename Container>
constexpr typename Container::const_iterator make_const(Container c, typename Container::iterator i)
{
    return typename Container::const_iterator(i);
}
} // namespace key

// using std::pair<> as SpreadKey for now
// for any further updates it could be anything
using DiagramKey = std::pair<int, int>;
