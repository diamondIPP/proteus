/**
 * \author Morit Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#include "noisescan.h"

#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(NoiseScan)

Analyzers::NoiseScan::NoiseScan(const Mechanics::Sensor& sensor,
                                const double bandwidth,
                                const double sigmaMax,
                                const double rateMax,
                                const Area& regionOfInterest,
                                TDirectory* parent,
                                const int binsOccupancy)
    : m_sensorId(sensor.id())
    , m_densityBandwidth(bandwidth)
    , m_sigmaMax(sigmaMax)
    , m_rateMax(rateMax)
    , m_numEvents(0)
{
  using namespace Utils;

  // region-of-interest must be bounded by the actual sensor size
  Area roi = intersection(regionOfInterest, sensor.sensitiveAreaPixel());

  DEBUG("input roi ", sensor.name(), ':');
  DEBUG("  col: ", regionOfInterest.interval(0));
  DEBUG("  row: ", regionOfInterest.interval(1));
  DEBUG("effective roi ", sensor.name(), ':');
  DEBUG("  col: ", roi.interval(0));
  DEBUG("  row: ", roi.interval(1));

  HistAxis axCol(roi.interval(0), roi.length(0), "Hit column");
  HistAxis axRow(roi.interval(1), roi.length(1), "Hit row");
  HistAxis axOcc(0, 1, binsOccupancy, "Pixel occupancy / hits/pixel/event");
  HistAxis axSig(0, 1, binsOccupancy, "Local significance");

  auto name = [&](const std::string& suffix) {
    return sensor.name() + '-' + suffix;
  };
  TDirectory* dir = makeDir(parent, "NoiseScan");
  m_occupancy = makeH2(dir, name("Occupancy"), axCol, axRow);
  m_occupancyDist = makeH1(dir, name("OccupancyDist"), axOcc);
  m_density = makeH2(dir, name("Density"), axCol, axRow);
  m_significance = makeH2(dir, name("LocalSignificance"), axCol, axRow);
  m_significanceDist = makeH1(dir, name("LocalSignificanceDist"), axSig);
  m_mask = makeH2(dir, name("Mask"), axCol, axRow);
}

std::string Analyzers::NoiseScan::name() const
{
  return "NoiseScan(sensorId=" + std::to_string(m_sensorId) + ')';
}

void Analyzers::NoiseScan::analyze(const Storage::Event& event)
{
  const Storage::Plane& plane = *event.getPlane(m_sensorId);

  for (Index i = 0; i < plane.numHits(); ++i) {
    const Storage::Hit& hit = *plane.getHit(i);
    m_occupancy->Fill(hit.col(), hit.row());
  }
  m_numEvents += 1;
}

/** Estimate the value at the given position from surrounding values.
 *
 * Use kernel density estimation w/ an Epanechnikov kernel to estimate the
 * density at the given point without using the actual value.
 */
static double
estimateDensityAtPosition(const TH2D* values, int i, int j, double bandwidth)
{
  assert((0 < i) && (i <= values->GetNbinsX()));
  assert((0 < j) && (j <= values->GetNbinsY()));

  double sumWeights = 0;
  double sumValues = 0;
  // with a bounded kernel only a subset of the points need to be considered.
  // define 2*bandwidth sized window around selected point to speed up
  // calculation.
  int bw = std::ceil(std::abs(bandwidth));
  int imin = std::max(1, i - bw);
  int imax = std::min(i + bw, values->GetNbinsX());
  int jmin = std::max(1, j - bw);
  int jmax = std::min(j + bw, values->GetNbinsY());
  for (int l = imin; l <= imax; ++l) {
    for (int m = jmin; m <= jmax; ++m) {
      if ((l == i) && (m == j))
        continue;

      double ui = (l - i) / bandwidth;
      double uj = (m - j) / bandwidth;
      double u2 = ui * ui + uj * uj;

      if (1 < u2)
        continue;

      // Epanechnikov kernel from:
      // https://en.wikipedia.org/wiki/Kernel_(statistics)
      double w = 3 * (1 - u2) / 4;

      sumWeights += w;
      sumValues += w * values->GetBinContent(l, m);
    }
  }
  return sumValues / sumWeights;
}

/** Write a smoothed density estimate to the density histogram. */
static void estimateDensity(const TH2D* values, double bandwidth, TH2D* density)
{
  assert(values->GetNbinsX() == density->GetNbinsX());
  assert(values->GetNbinsY() == density->GetNbinsY());

  for (int icol = 1; icol < values->GetNbinsX(); ++icol) {
    for (int irow = 1; irow < values->GetNbinsY(); ++irow) {
      auto den = estimateDensityAtPosition(values, icol, irow, bandwidth);
      density->SetBinContent(icol, irow, den);
    }
  }
  density->ResetStats();
  density->SetEntries(values->GetEntries());
}

void Analyzers::NoiseScan::finalize()
{
  estimateDensity(m_occupancy, m_densityBandwidth, m_density);
  // calculate local signifance, i.e. (hits - density) / sqrt(density)
  for (int icol = 1; icol <= m_occupancy->GetNbinsX(); ++icol) {
    for (int irow = 1; irow <= m_occupancy->GetNbinsY(); ++irow) {
      auto val = m_occupancy->GetBinContent(icol, irow);
      auto den = m_density->GetBinContent(icol, irow);
      auto sig = (val - den) / std::sqrt(den);
      m_significance->SetBinContent(icol, irow, sig);
    }
  }
  m_significance->ResetStats();
  m_significance->SetEntries(m_occupancy->GetEntries());
  // rescale hit counts to occupancy
  m_occupancy->Sumw2();
  m_occupancy->Scale(1.0 / m_numEvents);
  m_density->Scale(1.0 / m_numEvents);
  // fill per-pixel distributions
  Utils::fillDist(m_occupancy, m_occupancyDist);
  Utils::fillDist(m_significance, m_significanceDist);

  // select noisy pixels
  for (int icol = 1; icol <= m_significance->GetNbinsX(); ++icol) {
    for (int irow = 1; irow <= m_significance->GetNbinsY(); ++irow) {
      auto sig = m_significance->GetBinContent(icol, irow);
      auto rate = m_occupancy->GetBinContent(icol, irow);
      // pixel occupancy is a number of stddevs above local average
      bool isAboveRelative = (m_sigmaMax < sig);
      // pixel occupancy is above absolute limit
      bool isAboveAbsolute = (m_rateMax < rate);
      if (isAboveRelative || isAboveAbsolute) {
        m_mask->Fill(icol - 1, irow - 1);
      }
    }
  }

  INFO("noise scan sensor ", m_sensorId, ":");
  INFO("  cut relative: local mean + ", m_sigmaMax, " * local sigma");
  INFO("  cut absolute: ", m_rateMax, " hits/pixel/event");
  INFO("  max occupancy: ", m_occupancy->GetMaximum(), " hits/pixel/event");
  INFO("  noisy pixels: ", m_mask->GetEntries());
}

Mechanics::PixelMasks Analyzers::NoiseScan::constructMasks() const
{
  Mechanics::PixelMasks newMask;

  for (int icol = 1; icol <= m_mask->GetNbinsX(); ++icol) {
    for (int irow = 1; irow <= m_mask->GetNbinsY(); ++irow) {
      if (0 < m_mask->GetBinContent(icol, irow)) {
        // index of first data bin in ROOT histograms is 1
        newMask.maskPixel(m_sensorId, icol - 1, irow - 1);
      }
    }
  }
  return newMask;
}
