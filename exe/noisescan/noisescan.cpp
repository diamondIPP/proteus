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

NoiseScan::NoiseScan(const Mechanics::Sensor& sensor,
                     const double bandwidth,
                     const double sigmaMax,
                     const double rateMax,
                     const Area& regionOfInterest,
                     TDirectory* dir,
                     const int binsOccupancy)
    : m_sensorId(sensor.id())
    , m_sigmaMax(sigmaMax)
    , m_rateMax(rateMax)
    , m_numEvents(0)
{
  using namespace Utils;

  // adjust per-axis bandwith for pixel pitch along each axis such that the
  // covered area is approximately circular in metric coordinates.
  double scale = std::hypot(sensor.pitchCol(), sensor.pitchRow()) / M_SQRT2;
  m_bandwidthCol = std::ceil(bandwidth * scale / sensor.pitchCol());
  m_bandwidthRow = std::ceil(bandwidth * scale / sensor.pitchRow());

  DEBUG("pixel pitch scale: ", scale);
  DEBUG("bandwidth col: ", m_bandwidthCol);
  DEBUG("bandwidth row: ", m_bandwidthRow);

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

  TDirectory* sub = Utils::makeDir(dir, sensor.name() + "/noisescan");
  m_occupancy = makeH2(sub, "occupancy", axCol, axRow);
  m_occupancyDist = makeH1(sub, "occupancy_dist", axOcc);
  m_density = makeH2(sub, "density", axCol, axRow);
  m_significance = makeH2(sub, "local_significance", axCol, axRow);
  m_significanceDist = makeH1(sub, "local_significance_dist", axSig);
  m_mask = makeH2(sub, "mask", axCol, axRow);
}

std::string NoiseScan::name() const
{
  return "NoiseScan(sensorId=" + std::to_string(m_sensorId) + ')';
}

void NoiseScan::analyze(const Storage::Event& event)
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
estimateDensityAtPosition(const TH2D* values, int i, int j, int bwi, int bwj)
{
  assert((1 <= i) && (i <= values->GetNbinsX()));
  assert((1 <= j) && (j <= values->GetNbinsY()));
  assert(0 < bwi);
  assert(1 < bwj);

  double sumWeights = 0;
  double sumValues = 0;
  // with a bounded kernel only a subset of the gpoints need to be considered.
  // only 2*bandwidth sized window around selected point needs to be considered.
  int imin = std::max(1, i - bwi);
  int imax = std::min(i + bwi, values->GetNbinsX());
  int jmin = std::max(1, j - bwj);
  int jmax = std::min(j + bwj, values->GetNbinsY());
  for (int l = imin; l <= imax; ++l) {
    for (int m = jmin; m <= jmax; ++m) {
      if ((l == i) && (m == j))
        continue;

      double ui = (l - i) / static_cast<double>(bwi);
      double uj = (m - j) / static_cast<double>(bwj);
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
static void estimateDensity(const TH2D* values,
                            int bandwidthX,
                            int bandwidthY,
                            TH2D* density)
{
  assert(values->GetNbinsX() == density->GetNbinsX());
  assert(values->GetNbinsY() == density->GetNbinsY());

  for (int icol = 1; icol <= values->GetNbinsX(); ++icol) {
    for (int irow = 1; irow <= values->GetNbinsY(); ++irow) {
      auto den =
          estimateDensityAtPosition(values, icol, irow, bandwidthX, bandwidthY);
      density->SetBinContent(icol, irow, den);
    }
  }
  density->ResetStats();
  density->SetEntries(values->GetEntries());
}

void NoiseScan::finalize()
{
  estimateDensity(m_occupancy, m_bandwidthCol, m_bandwidthRow, m_density);
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
        m_mask->SetBinContent(icol, irow, 1);
      }
    }
  }

  INFO("noise scan sensor ", m_sensorId, ":");
  INFO("  cut relative: local mean + ", m_sigmaMax, " * local sigma");
  INFO("  cut absolute: ", m_rateMax, " hits/pixel/event");
  INFO("  max occupancy: ", m_occupancy->GetMaximum(), " hits/pixel/event");
  INFO("  noisy pixels: ", m_mask->GetEntries());
}

Mechanics::PixelMasks NoiseScan::constructMasks() const
{
  Mechanics::PixelMasks newMask;

  for (int icol = 1; icol <= m_mask->GetNbinsX(); ++icol) {
    for (int irow = 1; irow <= m_mask->GetNbinsY(); ++irow) {
      if (0 < m_mask->GetBinContent(icol, irow)) {
        auto col = static_cast<Index>(m_mask->GetXaxis()->GetBinLowEdge(icol));
        auto row = static_cast<Index>(m_mask->GetYaxis()->GetBinLowEdge(irow));
        newMask.maskPixel(m_sensorId, col, row);
      }
    }
  }
  return newMask;
}
