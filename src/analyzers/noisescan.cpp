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

using Utils::logger;

Analyzers::NoiseScan::NoiseScan(const Mechanics::Device& device,
                                        const toml::Value& cfg,
                                        TDirectory* parent)
    : m_numEvents(0)
{
  using Mechanics::Sensor;
  using Utils::Config::get;

  TDirectory* dir = parent->mkdir("NoiseScan");

  auto sensorIds = cfg.get<std::vector<int>>("sensor_ids");
  m_maxSigmaAboveAvg = get(cfg, "max_sigma_above_avg", 5.);
  m_maxOccupancy = get(cfg, "max_occupancy", 1.0);
  Area globalRoi = {
      {get(cfg, "col_min", INT_MIN), get(cfg, "col_max", INT_MAX)},
      {get(cfg, "row_min", INT_MIN), get(cfg, "row_max", INT_MAX)}};

  for (auto id = sensorIds.begin(); id != sensorIds.end(); ++id) {
    const Sensor* sensor = device.getSensor(*id);
    Sensor::Area sensorArea = sensor->sensitiveAreaPixel();
    // define region-of-interest by intersection of sensor sensorArea and cuts
    Area roi = Utils::intersection(globalRoi, sensorArea);

    auto makeDist = [&](const std::string& suffix) -> TH1D* {
      TH1D* h1 = new TH1D((sensor->name() + suffix).c_str(), "", 100, 0, 1);
      h1->SetDirectory(dir);
      return h1;
    };
    auto makeMap = [&](const std::string& suffix) -> TH2D* {
      TH2D* h2 = new TH2D((sensor->name() + suffix).c_str(),
                          "",
                          roi.axes[0].max - roi.axes[0].min,
                          roi.axes[0].min,
                          roi.axes[0].max,
                          roi.axes[1].max - roi.axes[1].min,
                          roi.axes[1].min,
                          roi.axes[1].max);
      h2->SetDirectory(dir);
      return h2;
    };

    m_sensorIds.push_back(*id);
    m_sensorRois.push_back(roi);
    m_occMaps.push_back(makeMap("-Occupancy"));
    m_occPixels.push_back(makeDist("-OccupancyPixels"));
    m_densities.push_back(makeMap("-Density"));
    m_pixelMasks.push_back(makeMap("-MaskedPixels"));
  }
}

std::string Analyzers::NoiseScan::name() const { return "NoiseScan"; }

void Analyzers::NoiseScan::analyze(const Storage::Event& event)
{
  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    Index id = m_sensorIds[i];
    const Storage::Plane& plane = *event.getPlane(id);
    const Area& roi = m_sensorRois[i];
    TH2D* occMap = m_occMaps[i];

    for (size_t j = 0; j < plane.numHits(); ++j) {
      const Storage::Hit& hit = *plane.getHit(j);
      if (roi.isInside(hit.col(), hit.row()))
        occMap->Fill(hit.col(), hit.row());
    }
  }
  m_numEvents += 1;
}

void Analyzers::NoiseScan::finalize()
{
  INFO('\n');

  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    auto id = m_sensorIds[i];
    auto roi = m_sensorRois[i];
    auto occMap = m_occMaps[i];
    auto occPixel = m_occPixels[i];
    auto pixelMask = m_pixelMasks[i];

    // rescale hit map to occupancy = hits / event
    occMap->Sumw2();
    occMap->Scale(1. / m_numEvents);
    // reset range for pixel histogram to maximum occupancy
    occPixel->SetBins(100, 0, occMap->GetMaximum());

    for (auto icol = occMap->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = occMap->GetNbinsY(); 0 < irow; --irow) {
        auto occ = occMap->GetBinContent(icol, irow);
        // only consider non-empty pixels
        if (0 < occ)
          occPixel->Fill(occ);
      }
    }

    double occMean = occPixel->GetMean();
    double occStd = occPixel->GetStdDev();
    double occMaxRel = occMean + m_maxSigmaAboveAvg * occStd;
    size_t numNoisyPixels = 0;
    for (auto icol = occMap->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = occMap->GetNbinsY(); 0 < irow; --irow) {
        double occ = occMap->GetBinContent(icol, irow);
        // pixel occupancy is a number of stddevs above global average
        bool isAboveRelative = (occMaxRel < occ);
        // pixel occupancy is above absolute limit
        bool isAboveAbsolute = (m_maxOccupancy < occ);

        if (isAboveRelative || isAboveAbsolute) {
          pixelMask->AddBinContent(pixelMask->GetBin(icol, irow));
          numNoisyPixels += 1;
        }
      }
    }
    pixelMask->SetEntries(numNoisyPixels);

    INFO("noise scan sensor ", id, ":\n");
    INFO("  roi col: ", roi.axes[0], '\n');
    INFO("  roi row: ", roi.axes[1], '\n');
    INFO("  max occupancy: ", occMap->GetMaximum(), " hits/event\n");
    INFO("  cut relative: ",
         occMaxRel,
         " hits/event == mean + ",
         m_maxSigmaAboveAvg,
         " * sigma\n");
    INFO("  cut absolute: ", m_maxOccupancy, " hits/event\n");
    INFO("  # noisy pixels: ", numNoisyPixels, '\n');
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
