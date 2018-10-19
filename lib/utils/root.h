/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#ifndef PT_ROOT_H
#define PT_ROOT_H

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>

#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>

namespace Utils {

/** Create a directory relative to the parent or return an existing one. */
TDirectory* makeDir(TDirectory* parent, const std::string& path);

/** Binning and label for a single histogram axis. */
struct HistAxis {
  const double limit0;
  const double limit1;
  const int bins;
  const std::string label;

  /** Construct w/ equal-sized bins in the given boundaries. */
  HistAxis(double a, double b, int n, std::string l = std::string())
      : limit0(a), limit1(b), bins(n), label(std::move(l))
  {
  }
  /** Construct w/ equal-size bins from an interval object.
   *
   * \param i Interval object that provides `.min()` and `.max()` accessors
   * \param n Number of bins
   * \param l Axis label
   */
  template <typename Interval>
  HistAxis(const Interval& i, int n, std::string l = std::string())
      : HistAxis(i.min(), i.max(), n, std::move(l))
  {
  }
  /** Construct w/ integer bins.
   *
   * Assumes that the histogram will be filled by integers in the range [a, b).
   * The number of bins is already defined by the boundaries and the bin
   * boundaries are placed such that the bin center corresponds to the integer
   * value.
   */
  explicit HistAxis(int a, int b, std::string l = std::string())
      : HistAxis(a - 0.5, b - 0.5, std::abs(b - a), std::move(l))
  {
  }
};

/** Create a named 1d histogram in the directory. */
TH1D* makeH1(TDirectory* dir, const std::string& name, HistAxis axis);

/** Create a named 2d histogram in the directory. */
TH2D* makeH2(TDirectory* dir,
             const std::string& name,
             HistAxis axis0,
             HistAxis axis1);

/** Create an unnamed 2d histogram that is not stored. */
TH1D* makeTransientH1(HistAxis axis);

/** Create an unnamed 2d histogram that is not stored. */
TH2D* makeTransientH2(HistAxis axis0, HistAxis axis1);

/** Fill a histogram with the values in each bin of the 2d histogram. */
void fillDist(const TH2D* values, TH1D* dist);

/** Returns the mean and variance restricted around the maximum of a histogram.
 *
 * \param histo The histogram
 * \param offset How many bins per side should be considered. If negative, the
 * whole histogram will be used. \return A std::pair with mean and variance
 */
std::pair<double, double> getRestrictedMean(const TH1D* histo,
                                            const int offset);

} // namespace Utils

#endif // PT_ROOT_H
