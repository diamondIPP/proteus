/**
 * \author Morit Kiehn <msmk@cern.ch>
 * \created 2016-08
 */

#include "noisescan.h"

#include <climits>

#include "mechanics/device.h"
#include "mechanics/noisemask.h"
#include "storage/event.h"
#include "storage/hit.h"
#include "storage/plane.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Analyzers::NoiseScan::NoiseScan(const Mechanics::Device& device,
                                const toml::Value& cfg,
                                TDirectory* parent)
    : m_numEvents(0)
{
  using Mechanics::Sensor;

  // clang-format off
  toml::Value defaults = toml::Table{
    {"density_bandwidth", 2.},
    {"max_sigma_above_avg", 5.},
    {"max_rate", 1.}};
  toml::Value defaultsSensor = toml::Table{
    {"col_min", INT_MIN},
    {"col_max", INT_MAX},
    {"row_min", INT_MIN},
    {"row_max", INT_MAX}};
  // clang-format on
  toml::Value c = Utils::Config::withDefaults(cfg, defaults);
  std::vector<toml::Value> s = Utils::Config::perSensor(cfg, defaultsSensor);

  TDirectory* dir = parent->mkdir("NoiseScan");

  m_densityBandwidth = c.find("density_bandwidth")->asNumber();
  m_maxSigmaAboveAvg = c.find("max_sigma_above_avg")->asNumber();
  m_maxRate = c.find("max_rate")->asNumber();

  // sensor specific
  for (auto cfgSensor = s.begin(); cfgSensor != s.end(); ++cfgSensor) {
    int id = cfgSensor->get<int>("id");
    const Mechanics::Sensor* sensor = device.getSensor(id);
    // roi is bounded by sensor size
    Area roi(Area::Axis(cfgSensor->get<int>("col_min"),
                        cfgSensor->get<int>("col_max")),
             Area::Axis(cfgSensor->get<int>("row_min"),
                        cfgSensor->get<int>("row_max")));
    roi = Utils::intersection(roi, sensor->sensitiveAreaPixel());
    addSensorHists(dir, id, sensor->name(), roi);
  }
}

void Analyzers::NoiseScan::addSensorHists(TDirectory* dir,
                                          int id,
                                          const std::string& name,
                                          const Area& roi)
{
  auto makeDist = [&](const std::string& suffix) -> TH1D* {
    TH1D* h1 = new TH1D((name + suffix).c_str(), "", 100, 0, 1);
    h1->SetDirectory(dir);
    return h1;
  };
  auto makeMap = [&](const std::string& suffix) -> TH2D* {
    TH2D* h2 = new TH2D((name + suffix).c_str(), "",
                        roi.axes[0].max - roi.axes[0].min, roi.axes[0].min,
                        roi.axes[0].max, roi.axes[1].max - roi.axes[1].min,
                        roi.axes[1].min, roi.axes[1].max);
    h2->SetDirectory(dir);
    return h2;
  };
  m_sensorIds.push_back(id);
  m_sensorRois.push_back(roi);
  m_occMaps.push_back(makeMap("-Occupancy"));
  m_occPixels.push_back(makeDist("-OccupancyPixels"));
  m_densities.push_back(makeMap("-DensityEstimate"));
  m_localSigmas.push_back(makeMap("-LocalSignificance"));
  m_localSigmasPixels.push_back(makeDist("-LocalSignificancePixels"));
  m_pixelMasks.push_back(makeMap("-MaskedPixels"));
}

std::string Analyzers::NoiseScan::name() const { return "NoiseScan"; }

void Analyzers::NoiseScan::analyze(const Storage::Event& event)
{
  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    Index id = m_sensorIds[i];
    const Storage::Plane& plane = *event.getPlane(id);
    const Area& roi = m_sensorRois[i];
    TH2D* occ = m_occMaps[i];

    for (size_t j = 0; j < plane.numHits(); ++j) {
      const Storage::Hit& hit = *plane.getHit(j);
      if (roi.isInside(hit.col(), hit.row()))
        occ->Fill(hit.col(), hit.row());
    }
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
  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    auto id = m_sensorIds[i];
    auto roi = m_sensorRois[i];
    auto occ = m_occMaps[i];
    auto occPixel = m_occPixels[i];
    auto density = m_densities[i];
    auto sigma = m_localSigmas[i];
    auto sigmaPixels = m_localSigmasPixels[i];
    auto pixelMask = m_pixelMasks[i];

    // estimate local density and noise significance
    for (auto icol = occ->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = occ->GetNbinsY(); 0 < irow; --irow) {
        auto nhits = occ->GetBinContent(icol, irow);
        auto den = estimateLocalDensity(occ, icol, irow, m_densityBandwidth);
        auto sig = (nhits - den) / std::sqrt(den);

        density->SetBinContent(icol, irow, den);
        sigma->SetBinContent(icol, irow, sig);
      }
    }
    sigma->SetEntries(occ->GetEntries());

    // fill per-pixel significance distribution
    sigmaPixels->SetBins(sigmaPixels->GetNbinsX(), sigma->GetMinimum(),
                         sigma->GetMaximum());
    for (auto icol = occ->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = occ->GetNbinsY(); 0 < irow; --irow) {
        sigmaPixels->Fill(sigma->GetBinContent(icol, irow));
      }
    }

    // rescale hit map to occupancy = hits / event
    occ->Sumw2();
    occ->Scale(1. / m_numEvents);

    // fill per-pixel rate histogram
    occPixel->SetBins(occPixel->GetNbinsX(), 0, occ->GetMaximum());
    for (auto icol = occ->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = occ->GetNbinsY(); 0 < irow; --irow) {
        auto rate = occ->GetBinContent(icol, irow);
        // only consider non-empty pixels
        if (0 < rate)
          occPixel->Fill(rate);
      }
    }

    // select noisy pixels
    size_t numNoisyPixels = 0;
    for (auto icol = sigma->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = sigma->GetNbinsY(); 0 < irow; --irow) {
        double sig = sigma->GetBinContent(icol, irow);
        double rate = occ->GetBinContent(icol, irow);
        // pixel occupancy is a number of stddevs above local average
        bool isAboveRelative = (m_maxSigmaAboveAvg < sig);
        // pixel occupancy is above absolute limit
        bool isAboveAbsolute = (m_maxRate < rate);

        if (isAboveRelative || isAboveAbsolute) {
          pixelMask->AddBinContent(pixelMask->GetBin(icol, irow));
          numNoisyPixels += 1;
        }
      }
    }
    pixelMask->SetEntries(numNoisyPixels);

    INFO("noise scan sensor ", id, ":");
    INFO("  roi col: ", roi.axes[0]);
    INFO("  roi row: ", roi.axes[1]);
    INFO("  max occupancy: ", occ->GetMaximum(), " hits/event");
    INFO("  cut relative: local mean + ", m_maxSigmaAboveAvg,
         " * local sigma");
    INFO("  cut absolute: ", m_maxRate, " hits/event");
    INFO("  noisy pixels: ", numNoisyPixels);
  }
}

void Analyzers::NoiseScan::writeMask(const std::string& path)
{
  Mechanics::NoiseMask newMask;

  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    Index id = m_sensorIds[i];
    TH2D* pixelMask = m_pixelMasks[i];

    for (auto icol = pixelMask->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = pixelMask->GetNbinsY(); 0 < irow; --irow) {
        if (0 < pixelMask->GetBinContent(icol, irow)) {
          // index of first data bin in ROOT histograms is 1
          newMask.maskPixel(id, icol - 1, irow - 1);
        }
      }
    }
  }
  newMask.writeFile(path);
}
