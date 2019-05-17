/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2018-12
 */

#ifndef PT_RANGE_H
#define PT_RANGE_H

#include <iterator>

namespace proteus {
namespace detail {

template <typename Iterator>
class Range {
public:
  constexpr Range(Iterator b, Iterator e) : m_begin(b), m_end(e) {}
  constexpr Iterator begin() { return m_begin; }
  constexpr Iterator end() { return m_end; }

private:
  Iterator m_begin;
  Iterator m_end;
};

template <typename Iterator>
constexpr Range<Iterator> makeRange(Iterator begin, Iterator end)
{
  return {begin, end};
}
template <typename Container>
constexpr auto makeRange(Container& c) -> Range<decltype(std::begin(c))>
{
  return {std::begin(c), std::end(c)};
}
template <typename Container>
constexpr auto makeRange(const Container& c) -> Range<decltype(std::begin(c))>
{
  return {std::begin(c), std::end(c)};
}

} // namespace detail
} // namespace proteus

#endif
