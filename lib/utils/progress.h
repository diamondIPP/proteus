/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-08
 */

#ifndef PT_PROGRESS_H
#define PT_PROGRESS_H

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Utils {

/** Display a progress indicator on a single output line */
class Progress {
public:
  /** Construct with the line length of the terminal. */
  Progress() : Progress(queryLineLength()) {}
  /** Construct with a fixed line length. */
  Progress(int lineLength)
      : m_os(std::cerr)
      , m_start(std::chrono::steady_clock::now())
      , m_lastUpdate(m_start)
      , m_length(lineLength)
  {
  }

  /** Update the progress indicator if necessary.
   *
   * \param processed  Number of processed items, must be in [0, total]
   * \param total      Total number of items
   */
  template <typename I, typename = typename std::is_integral<I>::type>
  void update(I processed, I total = std::numeric_limits<I>::max())
  {
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    // time-based redraw throttling w/ 30fps
    auto now = steady_clock::now();
    if (duration_cast<milliseconds>(now - m_lastUpdate) < milliseconds(32))
      return;

    // always show elapsed time
    auto hrs = duration_cast<std::chrono::hours>(now - m_start).count();
    auto min = duration_cast<std::chrono::minutes>(now - m_start).count();
    auto scn = duration_cast<std::chrono::seconds>(now - m_start).count();
    m_os << "elapsed ";
    m_os << std::setw(2) << std::setfill('0') << hrs << ':';
    m_os << std::setw(2) << std::setfill('0') << min << ':';
    m_os << std::setw(2) << std::setfill('0') << scn;
    // show fractional progress bar only if total number is known
    if (total < std::numeric_limits<I>::max()) {
      drawBar(processed, total);
    } else {
      drawNumber(processed);
    }
    // after printing, rewind back to the beginning of the line so that the
    // next update (or unrelated messages) can overwrite the current status
    m_os << '\r' << std::flush;
    m_lastUpdate = now;
  }
  /** Overwrite the progress indicator with empty spaces. */
  void clear()
  {
    for (int i = 0; i < m_length; ++i)
      m_os << ' ';
    m_os << '\r' << std::flush;
  }

private:
  /** Query the connected terminal for its line length. */
  static int queryLineLength()
  {
// from
// https://www.linuxquestions.org/questions/programming-9/get-width-height-of-a-terminal-window-in-c-810739/
#ifdef TIOCGSIZE
    struct ttysize ts;
    ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
    return ts.ts_cols;
#elif defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
    return ts.ws_col;
#else
    return 50; // fallback
#endif /* TIOCGSIZE */
  }

  template <typename I, typename = typename std::is_integral<I>::type>
  void drawBar(I current, I total)
  {
    int full = (m_length - 24);
    int bars = (full * current) / total;
    int percent = (100 * current) / total;

    m_os << " " << std::setw(3) << std::setfill(' ') << percent << "% ";
    m_os << "[";
    for (int i = 0; i < bars; ++i)
      m_os << '=';
    for (int i = bars; i < full; ++i)
      m_os << ' ';
    m_os << "]";
  }
  template <typename I, typename = typename std::is_integral<I>::type>
  void drawNumber(I current)
  {
    m_os << " processed" << std::setw(9) << std::setfill(' ') << current;
  }

  std::ostream& m_os;
  std::chrono::steady_clock::time_point m_start;
  std::chrono::steady_clock::time_point m_lastUpdate;
  int m_length;
};

} // namespace Utils

#endif // PT_PROGRESS_H
