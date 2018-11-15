/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#ifndef PT_ROOT_H
#define PT_ROOT_H

#include <cassert>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

#include <TDirectory.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>

namespace Utils {

using RootFilePtr = std::unique_ptr<TFile, void (*)(TFile*)>;

/** Open a ROOT file in read-only mode.
 *
 * \exception std::runtime_error When the file can not be opened.
 *
 * The resulting object will automatically close the file on destruction.
 */
RootFilePtr openRootRead(const std::string& path);

/** Open a ROOT file in write mode and overwrite existing content.
 *
 * \exception std::runtime_error When the file can not be opened.
 *
 * The resulting object will automatically write data to disk and close the
 * file on destruction. Pre-existing data will be overwritten.
 */
RootFilePtr openRootWrite(const std::string& path);

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
 * \param h1     1-d input histograms
 * \param offset How many additional bins should be be considered
 * \return The restricted mean and variance
 */
std::pair<double, double> getRestrictedMean(const TH1D* h1, int offset);

} // namespace Utils

#endif // PT_ROOT_H
