/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-08
 */

#ifndef __PROGRESSBAR__
#define __PROGRESSBAR__

#include <iomanip>
#include <iostream>

namespace Utils {

/** Display an updatable progress bar on a single output line */
class ProgressBar {
public:
  /**
   * \param lineLength  Total length of the output line
   */
  ProgressBar(size_t lineLength = 79)
      : m_length((lineLength < 8) ? 0 : (lineLength - 8))
      , m_curr(0)
      , m_os(std::cout)
  {
  }
  /** Update the progress bar if necessary. Fraction must be in [0,1]. */
  void update(float fraction)
  {
    size_t status = fraction * m_length;
    // only update if neccessary
    if (status == m_curr)
      return;
    m_os << "[";
    for (size_t i = 0; i < status; ++i)
      m_os << '=';
    for (size_t i = status; i < m_length; ++i)
      m_os << ' ';
    // after printing rewind back to the beginning of the line so that the next
    // update (or unrelated messages) overwrite the current status
    m_os << "] " << std::setw(3) << static_cast<size_t>(100 * fraction)
         << "%\r" << std::flush;
    // update for next iteration
    m_curr = status;
  }

private:
  const size_t m_length;
  size_t m_curr;
  std::ostream& m_os;
};

} // namespace Utils

#endif // __PROGRESSBAR__
