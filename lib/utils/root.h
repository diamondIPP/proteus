// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#pragma once

#include <cmath>
#include <memory>
#include <string>

#include <TDirectory.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>

namespace proteus {

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

/** Binning and labeling for a single histogram axis. */
struct HistAxis {
  const double limit0;
  const double limit1;
  const int bins;
  const std::string label;

  /** Construct w/ equal-sized bins in the given boundaries. */
  HistAxis(double a, double b, int n, std::string l = std::string());
  /** Construct w/ equal-size bins from an interval object.
   *
   * \param i Interval that provides `.min()` and `.max()` accessors
   * \param n Number of bins
   * \param l Axis label
   */
  template <typename Interval>
  HistAxis(const Interval& i, int n, std::string l = std::string());
  /** Construct w/ integer bins.
   *
   * Assumes that the histogram will be filled by integers in the range [a, b).
   * The number of bins is already defined by the boundaries and the bin
   * boundaries are placed such that the bin center corresponds to the integer
   * value.
   */
  static HistAxis Integer(int a, int b, std::string l = std::string());
  /** Construct w/ integer bins.
   *
   * \param i Interval that provides `.min()` and `.max()` as integers
   * \param l Axis label
   */
  template <typename Interval>
  static HistAxis Integer(const Interval& i, std::string l = std::string());
  /** Construct for the differences between two intervals.
   *
   * \param i0     First interval that provides `.min()` and `.max()`
   * \param pitch0 Bin pitch along the first interval axis
   * \param i1     Second interval that provides `.min()` and `.max()`
   * \param pitch1 Bin pitch along the second axis of the second interval
   *
   * The resulting axis can contain all possible differences (i1 - i0).
   * Binning is adjusted such that zero difference is at the center of a bin.
   */
  template <typename Interval0, typename Interval1>
  static HistAxis Difference(const Interval0& i0,
                             double pitch0,
                             const Interval1& i1,
                             double pitch1,
                             std::string l = std::string());
  /** Construct for differences within an interval.
   *
   * \param i     Interval that provides `.min()` and `.max()`
   * \param pitch Bin pitch along the interval axis
   */
  template <typename Interval>
  static HistAxis
  Difference(const Interval& i, double pitch, std::string l = std::string());
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

/** Fill a 1d histogram with the finite bin values from the 2d histogram. */
void fillDist(const TH2D* values, TH1D* dist);

/** Return the mean and variance restricted around the maximum of a histogram.
 *
 * \param h1     1-d input histograms
 * \param offset How many additional bins should be be considered
 * \return The restricted mean and variance
 */
std::pair<double, double> getRestrictedMean(const TH1D* h1, int offset);

// inline implementations

template <typename Interval>
inline HistAxis::HistAxis(const Interval& i, int n, std::string l)
    : HistAxis(i.min(), i.max(), n, std::move(l))
{
}

template <typename Interval>
inline HistAxis HistAxis::Integer(const Interval& i, std::string l)
{
  // ensure we end up with integer bins even w/ non-integer limits
  return Integer(std::floor(i.min()), std::ceil(i.max()), std::move(l));
}

template <typename Interval0, typename Interval1>
inline HistAxis HistAxis::Difference(const Interval0& i0,
                                     double pitch0,
                                     const Interval1& i1,
                                     double pitch1,
                                     std::string l)
{
  double dmin = i1.min() - i0.max();
  double dmax = i1.max() - i0.min();
  int bins = std::ceil((dmax - dmin) / std::min(pitch0, pitch1));

  // effective bin width (pitch) for the number of bins
  double pitch = (dmax - dmin) / bins;
  // nominal lower bin edge for the bin that contains zero difference
  double lower0 = dmin + pitch * std::floor((0 - dmin) / pitch);
  // target lower bin edge so that zero is at the bin center
  double target0 = -pitch / 2;
  double shift = target0 - lower0;
  // by construction the absolute shift is always less than one bin/pitch.
  // depending on its sign we need to add an additional bin at the lower or
  // upper edge and shift the bins to get zero at a bin center.
  if (shift < 0) {
    dmin += shift;
    dmax += (pitch + shift);
    bins += 1;
  } else if (0 < shift) {
    dmin -= (pitch - shift);
    dmax += shift;
    bins += 1;
  }
  // zero shift means the limits can stay as they are

  return {dmin, dmax, bins, std::move(l)};
}

template <typename Interval>
inline HistAxis
HistAxis::Difference(const Interval& i, double pitch, std::string l)
{
  return Difference(i, pitch, i, pitch, std::move(l));
}

} // namespace proteus
