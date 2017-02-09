/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-08
 */

#ifndef PT_PROGRESSBAR_H
#define PT_PROGRESSBAR_H

#include <iomanip>
#include <iostream>

namespace Utils {

/** Display an updatable progress bar on a single output line */
class ProgressBar {
public:
  /**
   * \param lineLength  Total length of the output line
   */
  ProgressBar(int lineLength = 79)
      : m_os(std::cout)
      , m_curr(-1)
      , m_max((lineLength < 9) ? 0 : (lineLength - 9))
  {
  }
  /** Update the progress bar if necessary. Fraction must be in [0,1]. */
  void update(float fraction)
  {
    int status = fraction * m_max;
    // only update if neccessary
    if (m_curr < status) {
      m_curr = status;
      display();
    }
  }
  /** Overwrite the progress bar with empty spaces. */
  void clear()
  {
    for (int i = 0; i <= (m_max + 8); ++i)
      m_os << ' ';
    m_os << '\r' << std::flush;
  }

private:
  /** Display the current progress status. */
  void display()
  {
    m_os << "[";
    for (int i = 0; i < m_curr; ++i)
      m_os << '=';
    for (int i = m_curr; i <= m_max; ++i)
      m_os << ' ';
    // after printing rewind back to the beginning of the line so that the
    // next update (or unrelated messages) overwrite the current status
    int percent = static_cast<int>(100.0f * m_curr / m_max);
    m_os << "] " << std::setw(3) << percent << "%\r" << std::flush;
  }

  std::ostream& m_os;
  int m_curr;
  int m_max;
};

} // namespace Utils

#endif // PT_PROGRESSBAR_H
