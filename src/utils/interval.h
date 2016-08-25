/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \created 2016-08
 */

#ifndef JU_INTERVAL_H
#define JU_INTERVAL_H

#include <algorithm>
#include <array>
#include <initializer_list>
#include <limits>

namespace Utils {

enum class Endpoints { CLOSED, OPEN, OPEN_MIN, OPEN_MAX };

/** An interval on a single ordered axes. */
template <typename T, Endpoints ENDPOINTS = Endpoints::OPEN_MAX>
struct Interval {
  T min, max;

  bool isEmpty() { return (min == max); }
  bool isInside(T x)
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
      : min(std::numeric_limits<T>::min())
      , max(std::numeric_limits<T>::max())
  {
  }
  /** Construct an interval with the given limits. */
  Interval(T a, T b)
      : min(std::min(a, b))
      , max(std::max(a, b))
  {
  }
};

template <size_t N, typename T, Endpoints ENDPOINTS = Endpoints::OPEN_MAX>
struct Box {
  typedef Interval<T, ENDPOINTS> Axis;
  typedef std::array<T, N> Point;

  std::array<Axis, N> axes;

  /** The Box is empty iff all axes are empty. */
  bool isEmpty()
  {
    bool status = true;
    for (size_t i = 0; i < N; ++i)
      status &= axes[i].isEmpty();
    return status;
  }
  bool isInside(Point x)
  {
    bool status = true;
    for (size_t i = 0; i < N; ++i)
      status &= axes[i].isInside(x[i]);
    return status;
  }

  Box(std::initializer_list<Axis> intervals)
  {
      std::copy(intervals.begin(), intervals.end(), axes.begin());
  }
};

} // namespace Utils

#endif // JU_INTERVAL_H
