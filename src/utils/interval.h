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

enum class Endpoints { Closed, Open, OpenMin, OpenMax };

/** Interval on a single ordered axes. */
template <typename T, Endpoints kEndpoints = Endpoints::OpenMax>
class Interval {
public:
  /** Construct an empty interval. */
  static Interval Empty()
  {
    return Interval(static_cast<T>(0), static_cast<T>(0));
  }
  /** Construct an interval spanning the full accessible range of the type T. */
  static Interval Unbounded()
  {
    return Interval(std::numeric_limits<T>::min(),
                    std::numeric_limits<T>::max());
  }
  /** Construct an interval with the given limits. */
  Interval(T a, T b) : m_min(std::min(a, b)), m_max(std::max(a, b)) {}

  T length() const { return m_max - m_min; }
  T min() const { return m_min; }
  T max() const { return m_max; }
  /** Check if the interval is empty.
   *
   * A closed interval can never be empty. With identical endpoints, the
   * endpoint(s) itself is the only element.
   */
  bool isEmpty() const
  {
    return (kEndpoints != Endpoints::Closed) && (m_min == m_max);
  }
  bool isInside(T x) const
  {
    // compiler should remove unused checks
    if (kEndpoints == Endpoints::Closed)
      return (m_min <= x) && (x <= m_max);
    if (kEndpoints == Endpoints::Open)
      return (m_min < x) && (x < m_max);
    if (kEndpoints == Endpoints::OpenMin)
      return (m_min < x) && (x <= m_max);
    // default is OpenMax
    return (m_min <= x) && (x < m_max);
  }

  /** Limit the interval to the intersection with the second interval. */
  template <typename U>
  void intersect(const Interval<U, kEndpoints>& other)
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
  void enclose(const Interval<U, kEndpoints>& other)
  {
    if (this->isEmpty()) {
      *this = other;
    } else if (other.isEmpty()) {
      // nothing to do
    } else {
      m_min = std::min<T>(m_min, other.min());
      m_max = std::max<T>(m_max, other.max());
    }
  }

private:
  Interval() : m_min(0), m_max(0) {}

  T m_min, m_max;

  template <size_t, typename, Endpoints>
  friend class Box;
};

/** N-dimensional aligned box defined by intervals along each axis. */
template <size_t N, typename T, Endpoints kEndpoints = Endpoints::OpenMax>
class Box {
public:
  typedef Interval<T, kEndpoints> AxisInterval;
  typedef std::array<T, N> Point;

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
  Box() : m_axes{} {}
  /** Construct a box from interval on each axis. */
  template <typename... Intervals,
            typename = typename std::enable_if<sizeof...(Intervals) == N>::type>
  Box(Intervals&&... axes) : m_axes{{std::forward<Intervals>(axes)...}}
  {
  }

  /** The full interval definition of the i-th axis. */
  const AxisInterval& interval(unsigned int i) const { return m_axes[i]; }
  /** The interval-length along the i-th axis. */
  T length(unsigned int i) const { return interval(i).length(); }
  /** The minimal value along the i-th axis. */
  T min(unsigned int i) const { return interval(i).min(); }
  /** The maximal value along the i-th axis. */
  T max(unsigned int i) const { return interval(i).max(); }

  /** The N-dimensional volume of the box. */
  T volume() const
  {
    double vol = m_axes[0].length();
    for (size_t i = 1; i < N; ++i)
      vol *= m_axes[i].length();
    return vol;
  }
  /** The box is empty if any axis is empty. */
  bool isEmpty() const
  {
    for (size_t i = 0; i < N; ++i)
      if (m_axes[i].isEmpty())
        return true;
    return false;
  }
  bool isInside(Point x) const
  {
    bool status = m_axes[0].isInside(x[0]);
    for (size_t i = 1; i < N; ++i)
      status &= m_axes[i].isInside(x[i]);
    return status;
  }
  template <typename... Us,
            typename = typename std::enable_if<sizeof...(Us) == N>::type>
  bool isInside(Us... x) const
  {
    return isInside(Point{{static_cast<T>(x)...}});
  }

  /** Limit the box to the intersection with the second box. */
  template <typename U>
  void intersect(const Box<N, U, kEndpoints>& other)
  {
    for (size_t i = 0; i < N; ++i)
      m_axes[i].intersect(other.interval(i));
  }
  /** Enlarge the box so that the second box is fully enclosed. */
  template <typename U>
  void enclose(const Box<N, U, kEndpoints>& other)
  {
    for (size_t i = 0; i < N; ++i)
      m_axes[i].enclose(other.interval(i));
  }

private:
  std::array<AxisInterval, N> m_axes;
};

/** Calculate maximum box that is contained in both input boxes. */
template <size_t N, typename T0, typename T1, Endpoints kEndpoints>
Box<N, T0, kEndpoints> intersection(const Box<N, T0, kEndpoints>& box0,
                                    const Box<N, T1, kEndpoints>& box1)
{
  Box<N, T0, kEndpoints> box = box0;
  box.intersect(box1);
  return box;
}

/** Calculate minimum bounding box that contains both input boxes. */
template <size_t N, typename T0, typename T1, Endpoints kEndpoints>
Box<N, T0, kEndpoints> boundingBox(const Box<N, T0, kEndpoints>& box0,
                                   const Box<N, T1, kEndpoints>& box1)
{
  Box<N, T0, kEndpoints> box = box0;
  box.enclose(box1);
  return box;
}

template <typename T, Endpoints kEndpoints>
std::ostream& operator<<(std::ostream& os,
                         const Interval<T, kEndpoints>& interval)
{
  if (kEndpoints == Endpoints::Closed) {
    os << '[' << interval.min() << ", " << interval.max() << ']';
  } else if (kEndpoints == Endpoints::Open) {
    os << '(' << interval.min() << ", " << interval.max() << ')';
  } else if (kEndpoints == Endpoints::OpenMin) {
    os << '(' << interval.min() << ", " << interval.max() << ']';
  } else /* OpenMax */ {
    os << '[' << interval.min() << ", " << interval.max() << ')';
  }
  return os;
}

} // namespace Utils

#endif // PT_INTERVAL_H
