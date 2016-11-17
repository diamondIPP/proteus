/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#ifndef PT_INTERVAL_H
#define PT_INTERVAL_H

#include <algorithm>
#include <array>
#include <initializer_list>
#include <limits>
#include <ostream>
#include <type_traits>

namespace Utils {

enum class Endpoints { CLOSED, OPEN, OPEN_MIN, OPEN_MAX };

/** An interval on a single ordered axes. */
template <typename T, Endpoints ENDPOINTS = Endpoints::OPEN_MAX>
struct Interval {
  T min, max;

  T length() const { return max - min; }
  /** Check if the interval is empty.
   *
   * A closed interval can never be empty. With identical endpoints, the
   * endpoint(s) itself is the only element.
   */
  bool isEmpty() const
  {
    return (ENDPOINTS != Endpoints::CLOSED) && (min == max);
  }
  bool isInside(T x) const
  {
    // compiler should remove unused checks
    if (ENDPOINTS == Endpoints::CLOSED)
      return (min <= x) && (x <= max);
    if (ENDPOINTS == Endpoints::OPEN)
      return (min < x) && (x < max);
    if (ENDPOINTS == Endpoints::OPEN_MIN)
      return (min < x) && (x <= max);
    // default is OPEN_MAX
    return (min <= x) && (x < max);
  }

  /** Construct the default interval encompassing the full range of T. */
  Interval()
      : min(std::numeric_limits<T>::min()), max(std::numeric_limits<T>::max())
  {
  }
  /** Construct an interval with the given limits. */
  Interval(T a, T b) : min(std::min(a, b)), max(std::max(a, b)) {}
};

/** N-dimensional box defined by intervals along each axis. */
template <size_t N, typename T, Endpoints ENDPOINTS = Endpoints::OPEN_MAX>
struct Box {
  typedef Interval<T, ENDPOINTS> Axis;
  typedef std::array<T, N> Point;

  std::array<Axis, N> axes;

  T volume() const
  {
    double vol = axes[0].length();
    for (size_t i = 1; i < N; ++i)
      vol *= axes[i].length();
    return vol;
  }
  /** The box is empty if any axis is empty. */
  bool isEmpty() const
  {
    for (size_t i = 0; i < N; ++i)
      if (axes[i].isEmpty())
        return true;
    return false;
  }
  bool isInside(Point x) const
  {
    bool status = axes[0].isInside(x[0]);
    for (size_t i = 1; i < N; ++i)
      status &= axes[i].isInside(x[i]);
    return status;
  }
  template <typename... Us,
            typename = typename std::enable_if<sizeof...(Us) == N>::type>
  bool isInside(Us... x) const
  {
    return isInside(Point{static_cast<T>(x)...});
  }

  Box() = default;
  template <typename... Intervals,
            typename = typename std::enable_if<sizeof...(Intervals) == N>::type>
  Box(Intervals&&... axes_) : axes{std::forward<Intervals>(axes_)...}
  {
  }
};

template <typename T0, typename T1, Endpoints ENDPOINTS>
Interval<T0, ENDPOINTS> intersection(const Interval<T0, ENDPOINTS>& interval0,
                                     const Interval<T1, ENDPOINTS>& interval1)
{
  return {std::max<T0>(interval0.min, interval1.min),
          std::min<T0>(interval0.max, interval1.max)};
}

template <size_t N, typename T0, typename T1, Endpoints ENDPOINTS>
Box<N, T0, ENDPOINTS> intersection(const Box<N, T0, ENDPOINTS>& box0,
                                   const Box<N, T1, ENDPOINTS>& box1)
{
  Box<N, T0, ENDPOINTS> box;
  for (size_t i = 0; i < N; ++i)
    box.axes[i] = intersection(box0.axes[i], box1.axes[i]);
  return box;
}

template <typename T, Endpoints ENDPOINTS>
std::ostream& operator<<(std::ostream& os,
                         const Interval<T, ENDPOINTS>& interval)
{
  if (ENDPOINTS == Endpoints::CLOSED) {
    os << '[' << interval.min << ", " << interval.max << ']';
  } else if (ENDPOINTS == Endpoints::OPEN) {
    os << '(' << interval.min << ", " << interval.max << ')';
  } else if (ENDPOINTS == Endpoints::OPEN_MIN) {
    os << '(' << interval.min << ", " << interval.max << ']';
  } else /* OPEN_MAX */ {
    os << '[' << interval.min << ", " << interval.max << ')';
  }
  return os;
}

} // namespace Utils

#endif // PT_INTERVAL_H
