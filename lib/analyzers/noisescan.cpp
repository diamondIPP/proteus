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

Analyzers::NoiseScan::NoiseScan(TDirectory* dir,
                                const Mechanics::Sensor& sensor,
                                const double bandwidth,
                                const double sigmaMax,
                                const double rateMax,
                                const Area& regionOfInterest,
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
  m_bandwidthCol = bandwidth * scale / sensor.pitchCol();
  m_bandwidthRow = bandwidth * scale / sensor.pitchRow();

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

  TDirectory* sub =
      Utils::makeDir(dir, "sensors/" + sensor.name() + "/noisescan");
  m_occupancy = makeH2(sub, "occupancy", axCol, axRow);
  m_occupancyDist = makeH1(sub, "occupancy_dist", axOcc);
  m_density = makeH2(sub, "density", axCol, axRow);
  m_significance = makeH2(sub, "local_significance", axCol, axRow);
  m_significanceDist = makeH1(sub, "local_significance_dist", axSig);
  m_maskAbsolute = makeH2(sub, "mask_absolute", axCol, axRow);
  m_maskRelative = makeH2(sub, "mask_relative", axCol, axRow);
}

std::string Analyzers::NoiseScan::name() const
{
  return "NoiseScan(sensorId=" + std::to_string(m_sensorId) + ')';
}

void Analyzers::NoiseScan::execute(const Storage::Event& event)
{
  const Storage::SensorEvent& sensorEvent = event.getSensorEvent(m_sensorId);

  for (Index i = 0; i < sensorEvent.numHits(); ++i) {
    const Storage::Hit& hit = sensorEvent.getHit(i);
    m_occupancy->Fill(hit.col(), hit.row());
  }
  m_numEvents += 1;
}

/** Estimate the value at the given position from surrounding values.
 *
 * Use kernel density estimation w/ an Epanechnikov kernel to estimate the
 * density at the given point without using the actual value.
 *
 * Positions with a non-zero entry in the mask will not be taken into account.
 */
static double estimateDensityAt(const TH2D* values,
                                const TH2D* mask,
                                int icol,
                                int jrow,
                                double bandwidthCol,
                                double bandwidthRow)
{
  assert(values);
  assert(mask);
  assert(values->GetNbinsX() == mask->GetNbinsX());
  assert(values->GetNbinsY() == mask->GetNbinsY());
  assert((1 <= icol) && (icol <= values->GetNbinsX()));
  assert((1 <= jrow) && (jrow <= values->GetNbinsY()));
  assert(0 < bandwidthCol);
  assert(0 < bandwidthRow);

  double sumWeights = 0;
  double sumValues = 0;
  // with a bounded kernel only a subset of all points need to be considered.
  // only 2*bandwidth sized window around selected point is needed.
  int imin = std::max<int>(std::floor(icol - bandwidthCol), 1);
  int imax = std::min<int>(std::ceil(icol + bandwidthCol), values->GetNbinsX());
  int jmin = std::max<int>(std::floor(jrow - bandwidthRow), 1);
  int jmax = std::min<int>(std::ceil(jrow + bandwidthRow), values->GetNbinsY());
  for (int i = imin; i <= imax; ++i) {
    for (int j = jmin; j <= jmax; ++j) {
      if ((i == icol) && (j == jrow))
        continue;
      if ((0 < mask->GetBinContent(i, j)))
        continue;

      double ui = (i - icol) / bandwidthCol;
      double uj = (j - jrow) / bandwidthRow;
      double u2 = ui * ui + uj * uj;

      if (1 < u2)
        continue;

      // Epanechnikov kernel from:
      // https://en.wikipedia.org/wiki/Kernel_(statistics)
      double w = 3 * (1 - u2) / 4;

      sumWeights += w;
      sumValues += w * values->GetBinContent(i, j);
    }
  }
  return sumValues / sumWeights;
}

/** Write a smoothed density estimate to the density histogram. */
static void estimateDensity(const TH2D* values,
                            const TH2D* mask,
                            double bandwidthCol,
                            double bandwidthRow,
                            TH2D* density)
{
  assert(values);
  assert(mask);
  assert(density);
  assert(values->GetNbinsX() == mask->GetNbinsX());
  assert(values->GetNbinsY() == mask->GetNbinsY());
  assert(values->GetNbinsX() == density->GetNbinsX());
  assert(values->GetNbinsY() == density->GetNbinsY());

  for (int i = 1; i <= values->GetNbinsX(); ++i) {
    for (int j = 1; j <= values->GetNbinsY(); ++j) {
      auto den =
          estimateDensityAt(values, mask, i, j, bandwidthCol, bandwidthRow);
      density->SetBinContent(i, j, den);
    }
  }
  density->ResetStats();
  density->SetEntries(values->GetEntries());
}

/** Calculate local signifance, i.e. (observed - expected) / sqrt(expected) */
static void computeSignificance(const TH2D* observed,
                                const TH2D* expected,
                                TH2D* significance)
{
  assert(observed->GetNbinsX() == expected->GetNbinsX());
  assert(observed->GetNbinsY() == expected->GetNbinsY());
  assert(observed->GetNbinsX() == significance->GetNbinsX());
  assert(observed->GetNbinsY() == significance->GetNbinsY());

  for (int i = 1; i <= observed->GetNbinsX(); ++i) {
    for (int j = 1; j <= observed->GetNbinsY(); ++j) {
      auto obs = observed->GetBinContent(i, j);
      auto exd = expected->GetBinContent(i, j);
      auto sig = (obs - exd) / std::sqrt(exd);
      significance->SetBinContent(i, j, sig);
    }
  }
  significance->ResetStats();
  significance->SetEntries(observed->GetEntries());
}

void Analyzers::NoiseScan::finalize()
{
  // mask pixels above the absolute rate cut
  auto entriesMax = m_numEvents * m_rateMax;
  for (int i = 1; i <= m_occupancy->GetNbinsX(); ++i) {
    for (int j = 1; j <= m_occupancy->GetNbinsY(); ++j) {
      if (entriesMax < m_occupancy->GetBinContent(i, j)) {
        m_maskAbsolute->SetBinContent(i, j, 1);
      }
    }
  }

  // compute rate significance using the local density
  estimateDensity(m_occupancy, m_maskAbsolute, m_bandwidthCol, m_bandwidthRow,
                  m_density);
  computeSignificance(m_occupancy, m_density, m_significance);
  // mask pixels above the local relative rate cut
  for (int i = 1; i <= m_significance->GetNbinsX(); ++i) {
    for (int j = 1; j <= m_significance->GetNbinsY(); ++j) {
      if (m_sigmaMax < m_significance->GetBinContent(i, j)) {
        m_maskRelative->SetBinContent(i, j, 1);
      }
    }
  }

  // rescale from hit counts to occupancy
  m_occupancy->Sumw2();
  m_occupancy->Scale(1.0 / m_numEvents);
  m_density->Sumw2();
  m_density->Scale(1.0 / m_numEvents);

  // fill per-pixel distributions
  Utils::fillDist(m_occupancy, m_occupancyDist);
  Utils::fillDist(m_significance, m_significanceDist);

  INFO("noise scan sensor ", m_sensorId, ":");
  INFO("  max occupancy: ", m_occupancy->GetMaximum(), " hits/pixel/event");
  INFO("  cut absolute: ", m_rateMax, " hits/pixel/event");
  INFO("  cut relative: local mean + ", m_sigmaMax, " * local sigma");
  INFO("  pixels masked absolute: ", m_maskAbsolute->GetEntries());
  INFO("  pixels masked relative: ", m_maskRelative->GetEntries());
}

Mechanics::PixelMasks Analyzers::NoiseScan::constructMasks() const
{
  Mechanics::PixelMasks pixelMask;

  auto extractMaskedPixels = [&](const TH2D* mask) {
    for (int i = 1; i <= mask->GetNbinsX(); ++i) {
      for (int j = 1; j <= mask->GetNbinsY(); ++j) {
        if (mask->GetBinContent(i, j) == 0)
          continue;
        // due to region-of-interest selection bin index != pixel index
        auto col = static_cast<Index>(mask->GetXaxis()->GetBinLowEdge(i));
        auto row = static_cast<Index>(mask->GetYaxis()->GetBinLowEdge(j));
        pixelMask.maskPixel(m_sensorId, col, row);
      }
    }
  };
  extractMaskedPixels(m_maskAbsolute);
  extractMaskedPixels(m_maskRelative);
  return pixelMask;
}
