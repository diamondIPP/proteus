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
  using Size = uint64_t;

  /** Construct with the line length of the terminal.
   *
   * \param total Total number of items
   *
   * Which progress indicator is shown depends on the total number of items.
   *
   * *   `std::numeric_limits<Size>::min()` means the progress indicator is
   *     disabled and nothing is shown.
   * *   `std::numeric_limits<Size>::max()` means tht total number of items
   *     is unknown. The current item number and the elapsed time is shown.
   * *   For all other values, the relative progress and a progress bar is
   *     shown.
   */
  Progress(Size total = std::numeric_limits<Size>::max())
      : m_os(std::cerr)
      , m_start(std::chrono::steady_clock::now())
      , m_lastUpdate(m_start)
      , m_total(total)
      , m_length(queryLineLength())
  {
  }

  /** Update the progress indicator if necessary.
   *
   * \param processed  Number of processed items, must be in [0, total]
   */
  void update(Size processed)
  {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    // only continue if enabled
    if (m_total == std::numeric_limits<Size>::min())
      return;
    // time-based redraw throttling w/ 30fps
    auto now = steady_clock::now();
    if (duration_cast<milliseconds>(now - m_lastUpdate) < milliseconds(32))
      return;

    // always show elapsed time
    auto hrs = duration_cast<std::chrono::hours>(now - m_start).count();
    auto min = duration_cast<std::chrono::minutes>(now - m_start).count() % 60;
    auto scn = duration_cast<std::chrono::seconds>(now - m_start).count() % 60;
    m_os << "elapsed ";
    m_os << std::setw(2) << std::setfill('0') << hrs << ':';
    m_os << std::setw(2) << std::setfill('0') << min << ':';
    m_os << std::setw(2) << std::setfill('0') << scn;
    // show fractional progress bar only if total number is known
    if (m_total < std::numeric_limits<Size>::max()) {
      drawBar(processed);
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
    // only relevant if enabled
    if (m_total == std::numeric_limits<Size>::min())
      return;

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

  void drawBar(Size current)
  {
    int full = (m_length - 24);
    int bars = (full * current) / m_total;
    int percent = (100 * current) / m_total;

    m_os << " " << std::setw(3) << std::setfill(' ') << percent << "% ";
    m_os << "[";
    for (int i = 0; i < bars; ++i)
      m_os << '=';
    for (int i = bars; i < full; ++i)
      m_os << ' ';
    m_os << "]";
  }
  void drawNumber(Size current)
  {
    m_os << " processed" << std::setw(9) << std::setfill(' ') << current;
  }

  std::ostream& m_os;
  std::chrono::steady_clock::time_point m_start;
  std::chrono::steady_clock::time_point m_lastUpdate;
  Size m_total;
  int m_length;
};

} // namespace Utils

#endif // PT_PROGRESS_H
