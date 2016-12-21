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

Analyzers::NoiseScan::NoiseScan(const Mechanics::Device& device,
                                const Index sensorId,
                                const double bandwidth,
                                const double m_sigmaMax,
                                const double rateMax,
                                const Area& regionOfInterest,
                                TDirectory* parent,
                                const int binsOccupancy)
    : m_sensorId(sensorId)
    , m_densityBandwidth(bandwidth)
    , m_sigmaAboveMeanMax(m_sigmaMax)
    , m_rateMax(rateMax)
    , m_numEvents(0)
{
  using namespace Utils;

  TDirectory* dir = Utils::makeDir(parent, "NoiseScan");
  const Mechanics::Sensor& sensor = *device.getSensor(sensorId);
  auto name = [&](const std::string& suffix) {
    return sensor.name() + '-' + suffix;
  };

  // region of interest is bounded by the active sensor size
  m_roi = Utils::intersection(regionOfInterest, sensor.sensitiveAreaPixel());

  HistAxis axCol(m_roi.interval(0), m_roi.length(0), "Hit column");
  HistAxis axRow(m_roi.interval(1), m_roi.length(1), "Hit row");
  HistAxis axOcc(0, 1, binsOccupancy, "Pixel occupancy");
  HistAxis axSig(0, 1, binsOccupancy, "Local pixel rate significance");

  m_occ = makeH2(dir, name("Occupancy"), axCol, axRow);
  m_occPixels = makeH1(dir, name("OccupancyPixels"), axOcc);
  m_density = makeH2(dir, name("DensityEstimate"), axCol, axRow);
  m_sigma = makeH2(dir, name("LocalSignificance"), axCol, axRow);
  m_sigmaPixels = makeH1(dir, name("LocalSignificancePixels"), axSig);
  m_maskedPixels = makeH2(dir, name("MaskedPixels"), axCol, axRow);
}

std::string Analyzers::NoiseScan::name() const
{
  return "NoiseScan(sensorId=" + std::to_string(m_sensorId) + ')';
}

void Analyzers::NoiseScan::analyze(const Storage::Event& event)
{
  const Storage::Plane& plane = *event.getPlane(m_sensorId);

  for (size_t j = 0; j < plane.numHits(); ++j) {
    const Storage::Hit& hit = *plane.getHit(j);
    if (m_roi.isInside(hit.col(), hit.row()))
      m_occ->Fill(hit.col(), hit.row());
  }
  m_numEvents += 1;
}

/** Estimate the value at the given position from surrounding values.
 *
 * Use kernel density estimation w/ an Epanechnikov kernel to estimate the
 * density at the given point without using the actual value.
 */
static double
estimateLocalDensity(const TH2D* values, int i, int j, double bandwidth)
{
  double sumWeights = 0;
  double sumValues = 0;
  // with a bounded kernel only a subset of the points need to be considered.
  // define 2*bandwidth+1 sized window around selected point to speed up
  // calculation.
  int bw = std::ceil(std::abs(bandwidth));
  int imin = std::max(1, i - bw);
  int imax = std::min(i + bw, values->GetNbinsX());
  int jmin = std::max(1, j - bw);
  int jmax = std::min(j + bw, values->GetNbinsY());
  for (int l = imin; l < imax; ++l) {
    for (int m = jmin; m < jmax; ++m) {
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

void Analyzers::NoiseScan::finalize()
{
  // estimate local density and noise significance
  for (auto icol = m_occ->GetNbinsX(); 0 < icol; --icol) {
    for (auto irow = m_occ->GetNbinsY(); 0 < irow; --irow) {
      auto nhits = m_occ->GetBinContent(icol, irow);
      auto den = estimateLocalDensity(m_occ, icol, irow, m_densityBandwidth);
      auto sig = (nhits - den) / std::sqrt(den);

      m_density->SetBinContent(icol, irow, den);
      m_sigma->SetBinContent(icol, irow, sig);
    }
  }
  m_sigma->SetEntries(m_occ->GetEntries());

  // fill per-pixel significance distribution
  m_sigmaPixels->SetBins(m_sigmaPixels->GetNbinsX(), m_sigma->GetMinimum(),
                         m_sigma->GetMaximum());
  for (auto icol = m_sigma->GetNbinsX(); 0 < icol; --icol) {
    for (auto irow = m_sigma->GetNbinsY(); 0 < irow; --irow) {
      m_sigmaPixels->Fill(m_sigma->GetBinContent(icol, irow));
    }
  }

  // rescale hit map to occupancy = hits / event
  m_occ->Sumw2();
  m_occ->Scale(1. / m_numEvents);

  // fill per-pixel rate histogram
  m_occPixels->SetBins(m_occPixels->GetNbinsX(), 0, m_occ->GetMaximum());
  for (auto icol = m_occ->GetNbinsX(); 0 < icol; --icol) {
    for (auto irow = m_occ->GetNbinsY(); 0 < irow; --irow) {
      auto rate = m_occ->GetBinContent(icol, irow);
      // only consider non-empty pixels
      if (0 < rate)
        m_occPixels->Fill(rate);
    }
  }

  // select noisy pixels
  size_t numNoisyPixels = 0;
  for (auto icol = m_sigma->GetNbinsX(); 0 < icol; --icol) {
    for (auto irow = m_sigma->GetNbinsY(); 0 < irow; --irow) {
      double sig = m_sigma->GetBinContent(icol, irow);
      double rate = m_occ->GetBinContent(icol, irow);
      // pixel occupancy is a number of stddevs above local average
      bool isAboveRelative = (m_sigmaAboveMeanMax < sig);
      // pixel occupancy is above absolute limit
      bool isAboveAbsolute = (m_rateMax < rate);

      if (isAboveRelative || isAboveAbsolute) {
        m_maskedPixels->AddBinContent(m_maskedPixels->GetBin(icol, irow));
        numNoisyPixels += 1;
      }
    }
  }
  m_maskedPixels->SetEntries(numNoisyPixels);

  INFO("noise scan sensor ", m_sensorId, ":");
  INFO("  roi col: ", m_roi.interval(0));
  INFO("  roi row: ", m_roi.interval(1));
  INFO("  max occupancy: ", m_occ->GetMaximum(), " hits/event");
  INFO("  cut relative: local mean + ", m_sigmaAboveMeanMax, " * local sigma");
  INFO("  cut absolute: ", m_rateMax, " hits/event");
  INFO("  noisy pixels: ", numNoisyPixels);
}

Mechanics::NoiseMask Analyzers::NoiseScan::constructMask() const
{
  Mechanics::NoiseMask newMask;

  for (auto icol = m_maskedPixels->GetNbinsX(); 0 < icol; --icol) {
    for (auto irow = m_maskedPixels->GetNbinsY(); 0 < irow; --irow) {
      if (0 < m_maskedPixels->GetBinContent(icol, irow)) {
        // index of first data bin in ROOT histograms is 1
        newMask.maskPixel(m_sensorId, icol - 1, irow - 1);
      }
    }
  }
  return newMask;
}
