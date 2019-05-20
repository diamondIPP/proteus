/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <ostream>
#include <type_traits>

namespace proteus {

/** Interval on a single ordered axes.
 *
 * The interval is half-open, i.e. the lower bound is included while the
 * upper bound is excluded.
 */
template <typename T>
class Interval {
public:
  /** Construct an empty interval. */
  static constexpr Interval Empty()
  {
    return Interval(static_cast<T>(0), static_cast<T>(0));
  }
  /** Construct an interval spanning the full accessible range of the type T. */
  static constexpr Interval Unbounded()
  {
    return Interval(std::numeric_limits<T>::min(),
                    std::numeric_limits<T>::max());
  }
  /** Construct an interval with the given limits. */
  constexpr Interval(T a, T b) : m_min((a < b) ? a : b), m_max((a < b) ? b : a)
  {
  }
  /** Construct from an interval of another type. */
  template <typename U>
  constexpr Interval(const Interval<U>& other)
      : m_min(other.min()), m_max(other.max())
  {
  }

  constexpr T length() const { return m_max - m_min; }
  constexpr T min() const { return m_min; }
  constexpr T max() const { return m_max; }
  /** Check if the interval is empty. */
  constexpr bool isEmpty() const { return (m_min == m_max); }
  constexpr bool isInside(T x) const { return (m_min <= x) and (x < m_max); }

  /** Limit the interval to the intersection with the second interval. */
  template <typename U>
  void intersect(const Interval<U>& other)
  {
    if (other.isEmpty()) {
      // there is no intersection w/ an empty interval
      *this = Empty();
    } else {
      m_min = std::max<T>(m_min, other.min());
      m_max = std::min<T>(m_max, other.max());
      // there is no overlap between the intervals
      if (m_max < m_min) {
        *this = Empty();
      }
    }
  }
  /** Enlarge the interval so that the second interval is fully enclosed. */
  template <typename U>
  void enclose(const Interval<U>& other)
  {
    if (this->isEmpty()) {
      *this = other;
    } else if (!other.isEmpty()) {
      m_min = std::min<T>(m_min, other.min());
      m_max = std::max<T>(m_max, other.max());
    }
  }
  /** Enlarge the interval on each side by the given amount. */
  void enlarge(T extra)
  {
    m_min -= extra;
    m_max += extra;
  }

private:
  Interval() : m_min(0), m_max(0) {}

  T m_min, m_max;

  template <size_t, typename>
  friend class Box;
};

/** N-dimensional aligned box defined by intervals along each axis.
 *
 * The intervals along each axis are half-open, i.e. the lower bound is
 * included while the upper bound is excluded.
 */
template <size_t N, typename T>
class Box {
public:
  using AxisInterval = Interval<T>;

  /** Construct an empty box. */
  static Box Empty()
  {
    Box box;
    std::fill(box.m_axes.begin(), box.m_axes.end(), AxisInterval::Empty());
    return box;
  }
  /** Construct a box spanning the available range of type T. */
  static Box Unbounded()
  {
    Box box;
    std::fill(box.m_axes.begin(), box.m_axes.end(), AxisInterval::Unbounded());
    return box;
  }
  /** Default box is empty.
   *
   * \warning This is only public to make the compiler happy. Use `Empty()`.
   */
  constexpr Box() : m_axes{} {}
  /** Construct a box from interval on each axis. */
  template <typename... Intervals,
            typename = typename std::enable_if<sizeof...(Intervals) == N>::type>
  constexpr Box(Intervals&&... axes)
      : m_axes{{std::forward<Intervals>(axes)...}}
  {
  }

  /** The full interval definition of the i-th axis. */
  constexpr AxisInterval interval(size_t i) const { return m_axes.at(i); }
  /** The interval-length along the i-th axis. */
  constexpr T length(size_t i) const { return m_axes.at(i).length(); }
  /** The minimal value along the i-th axis. */
  constexpr T min(size_t i) const { return m_axes.at(i).min(); }
  /** The maximal value along the i-th axis. */
  constexpr T max(size_t i) const { return m_axes.at(i).max(); }

  /** The N-dimensional volume of the box. */
  T volume() const
  {
    T vol = static_cast<T>(1);
    for (size_t i = 0; i < N; ++i) {
      vol *= m_axes[i].length();
    }
    return vol;
  }
  /** The box is empty if any axis is empty. */
  bool isEmpty() const
  {
    return std::any_of(m_axes.begin(), m_axes.end(),
                       [](const AxisInterval& i) { return i.isEmpty(); });
  }
  /** Check if the N-dimensional point is in the box. */
  template <typename... Us,
            typename = typename std::enable_if<sizeof...(Us) == N>::type>
  bool isInside(Us... x) const
  {
    std::array<T, N> point = {static_cast<T>(x)...};
    bool status = true;
    for (size_t i = 0; i < N; ++i) {
      status &= m_axes[i].isInside(point[i]);
    }
    return status;
  }

  /** Limit the box to the intersection with the second box. */
  template <typename U>
  void intersect(const Box<N, U>& other)
  {
    for (size_t i = 0; i < N; ++i) {
      m_axes[i].intersect(other.interval(i));
    }
  }
  /** Enlarge the box so that the second box is fully enclosed. */
  template <typename U>
  void enclose(const Box<N, U>& other)
  {
    for (size_t i = 0; i < N; ++i) {
      m_axes[i].enclose(other.interval(i));
    }
  }
  /** Enlarge the box along both directions on each axis by the given amount. */
  void enlarge(T extra)
  {
    for (size_t i = 0; i < N; ++i) {
      m_axes[i].enlarge(extra);
    }
  }

private:
  std::array<AxisInterval, N> m_axes;
};

/** Calculate maximum box that is contained in both input boxes. */
template <size_t N, typename T0, typename T1>
inline Box<N, T0> intersection(const Box<N, T0>& box0, const Box<N, T1>& box1)
{
  Box<N, T0> box = box0;
  box.intersect(box1);
  return box;
}

/** Calculate minimum bounding box that contains both input boxes. */
template <size_t N, typename T0, typename T1>
inline Box<N, T0> boundingBox(const Box<N, T0>& box0, const Box<N, T1>& box1)
{
  Box<N, T0> box = box0;
  box.enclose(box1);
  return box;
}

template <size_t N, typename T, typename U>
inline Box<N, T> enlarged(const Box<N, T>& box, U extra)
{
  Box<N, T> larger = box;
  larger.enlarge(extra);
  return larger;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const Interval<T>& interval)
{
  os << '[' << interval.min() << ", " << interval.max() << ')';
  return os;
}

} // namespace proteus
