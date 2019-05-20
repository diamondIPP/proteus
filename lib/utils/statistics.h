// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-09
 */

#pragma once

#include <cmath>
#include <limits>
#include <ostream>

namespace proteus {

/** Accumulate summary statistics for a single variable. */
template <typename T>
class StatAccumulator {
public:
  StatAccumulator()
      : m_entries(0)
      , m_avg(0)
      , m_m2(0)
      , m_min(std::numeric_limits<T>::max())
      , m_max(std::numeric_limits<T>::min())
  {
  }
  void fill(T val)
  {
    // from <https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance>
    double delta = val - m_avg;
    m_entries += 1;
    m_avg += delta / m_entries;
    m_m2 += delta * (val - m_avg);
    m_min = std::min(m_min, val);
    m_max = std::max(m_max, val);
  }

  double avg() const { return m_avg; }
  double var() const
  {
    if (m_entries < 2)
      return std::numeric_limits<double>::quiet_NaN();
    return m_m2 / (m_entries - 1);
  }
  double std() const { return std::sqrt(var()); }
  T min() const { return m_min; }
  T max() const { return m_max; }

private:
  uint64_t m_entries;
  double m_avg, m_m2;
  T m_min, m_max;
};

// inline implementations

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const StatAccumulator<T>& acc)
{
  os << acc.avg() << " (std=" << acc.std() << ", min=" << acc.min()
     << ", max=" << acc.max() << ")";
  return os;
}

} // namespace proteus
