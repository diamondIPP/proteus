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

/** Fill a histogram with the values in each bin of the 2d histogram.
 *
 * The binning range of the output histograms are adjusted to the actual limits
 * while the number of bins are kept fixed.
 */
void fillDist(const TH2D* values, TH1D* dist);

/**
 * Returns the mean and variance restricted around the maximum of a histogram.
 * @param histo The histogram
 * @param offset How many bins per side should be considered. If negative, the whole histogram will be used.
 * @return A std::pair with mean and variance
 */
std::pair<double, double> getRestrictedMean(const TH1D* histo, const int offset);

} // namespace Utils

// implementation

inline TDirectory* Utils::makeDir(TDirectory* parent, const std::string& path)
{
  assert(parent && "Parent directory must be non-NULL");

  TDirectory* dir = parent->GetDirectory(path.c_str());
  if (!dir) {
    // the return value of this is useless:
    // NULL means both the directory exists and everything is ok, or a failure.
    // not-NULL returns only the first subdirectory if path defines a hierachy,
    // but we want the final directory that was created.
    dir = parent->mkdir(path.c_str());
    dir = parent->GetDirectory(path.c_str());
  }
  if (!dir)
    throw std::runtime_error("Could not create ROOT directory '" + path + '\'');
  return dir;
}

inline TH1D*
Utils::makeH1(TDirectory* dir, const std::string& name, HistAxis axis)
{
  assert(dir && "Directory must be non-NULL");

  TH1D* h = new TH1D(name.c_str(), "", axis.bins, axis.limit0, axis.limit1);
  h->SetXTitle(axis.label.c_str());
  h->SetDirectory(dir);
  return h;
}

inline TH2D* Utils::makeH2(TDirectory* dir,
                           const std::string& name,
                           HistAxis axis0,
                           HistAxis axis1)
{
  assert(dir && "Directory must be non-NULL");

  TH2D* h = new TH2D(name.c_str(), "", axis0.bins, axis0.limit0, axis0.limit1,
                     axis1.bins, axis1.limit0, axis1.limit1);
  h->SetXTitle(axis0.label.c_str());
  h->SetYTitle(axis1.label.c_str());
  h->SetDirectory(dir);
  return h;
}

inline TH1D* Utils::makeTransientH1(HistAxis axis)
{
  // try to generate a (unique) name. not sure if needed
  std::string name;
  name += axis.label;
  name += std::to_string(axis.limit0);
  name += std::to_string(axis.limit1);
  name += std::to_string(axis.bins);

  TH1D* h = new TH1D(name.c_str(), "", axis.bins, axis.limit0, axis.limit1);
  h->SetXTitle(axis.label.c_str());
  h->SetDirectory(nullptr);
  return h;
}

inline TH2D* Utils::makeTransientH2(HistAxis axis0, HistAxis axis1)
{
  // try to generate a (unique) name. not sure if needed
  std::string name;
  name += axis0.label;
  name += std::to_string(axis0.limit0);
  name += std::to_string(axis0.limit1);
  name += std::to_string(axis0.bins);
  name += axis1.label;
  name += std::to_string(axis1.limit0);
  name += std::to_string(axis1.limit1);
  name += std::to_string(axis1.bins);

  TH2D* h = new TH2D("", "", axis0.bins, axis0.limit0, axis0.limit1, axis1.bins,
                     axis1.limit0, axis1.limit1);
  h->SetXTitle(axis0.label.c_str());
  h->SetYTitle(axis1.label.c_str());
  h->SetDirectory(nullptr);
  return h;
}

inline void Utils::fillDist(const TH2D* values, TH1D* dist)
{
  // ensure all values are binned
  dist->SetBins(dist->GetNbinsX(), values->GetMinimum(),
                std::nextafter(values->GetMaximum(), values->GetMaximum() + 1));
  for (int icol = 1; icol <= values->GetNbinsX(); ++icol) {
    for (int irow = 1; irow <= values->GetNbinsY(); ++irow) {
      auto value = values->GetBinContent(icol, irow);
      if (std::isfinite(value))
        dist->Fill(value);
    }
  }
}

inline std::pair<double, double> Utils::getRestrictedMean(const TH1D* histo, const int offset)
{
  TH1D* h = new TH1D(*histo);
  int maxBin = h->GetMaximumBin();
  if (offset >= 0) {
    h->GetXaxis()->SetRange(maxBin - offset, maxBin + offset);
  }
  double mean = h->GetMean();
  double var = h->GetMeanError() * h->GetMeanError();

  delete h;
  return std::make_pair(mean, var);
}

#endif // PT_ROOT_H
