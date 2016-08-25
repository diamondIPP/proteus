/**
 * \author Morit Kiehn <msmk@cern.ch>
 * \created 2016-08
 */

#include "noise.h"

#include <climits>

#include "mechanics/device.h"
#include "mechanics/noisemask.h"
#include "storage/event.h"
#include "storage/hit.h"
#include "storage/plane.h"
#include "utils/logger.h"

using Utils::logger;

Analyzers::NoiseAnalyzer::NoiseAnalyzer(const std::vector<int>& sensorIds,
                                        double maxSigmaAboveAvg,
                                        double maxOccupancy,
                                        SensorRoi roi,
                                        const std::string& outputNoiseMask)
    : m_sensorIds(sensorIds)
    , m_numEvents(0)
    , m_maxSigmaAboveAvg(maxSigmaAboveAvg)
    , m_maxOccupancy(maxOccupancy)
    , m_roi(roi)
    , m_outputNoiseMask(outputNoiseMask)
{
}

std::unique_ptr<Analyzers::NoiseAnalyzer>
Analyzers::NoiseAnalyzer::make(const toml::Value& cfg,
                               const Mechanics::Device& device,
                               const std::string& outputNoiseMask,
                               TDirectory* outputHists)
{
  using Utils::Config::get;

  auto sensorIds = cfg.get<std::vector<int>>("sensor_ids");
  auto maxSigmaAboveAvg = get(cfg, "max_sigma_above_avg", 5.);
  auto maxOccupancy = get(cfg, "max_occupancy", 1.0);
  SensorRoi roi = {SensorRoi::Axis(get(cfg, "col_min", INT_MIN),
                                   get(cfg, "col_max", INT_MAX)),
                   SensorRoi::Axis(get(cfg, "row_min", INT_MIN),
                                   get(cfg, "row_max", INT_MAX))};

  std::unique_ptr<NoiseAnalyzer> na(new NoiseAnalyzer(
      sensorIds, maxSigmaAboveAvg, maxOccupancy, roi, outputNoiseMask));

  na->initialize(device, outputHists);

  return std::move(na);
}

std::string Analyzers::NoiseAnalyzer::name() const { return "NoiseAnalyzer"; }

void Analyzers::NoiseAnalyzer::initialize(const Mechanics::Device& device,
                                          TDirectory* outputHists)
{
  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    auto id = m_sensorIds[i];
    auto sensor = device.getSensor(id);

    auto occMap = new TH2D((sensor->getName() + "-occupancy_map").c_str(),
                           "",
                           sensor->getNumX(),
                           0,
                           sensor->getNumX(),
                           sensor->getNumY(),
                           0,
                           sensor->getNumY());
    occMap->SetDirectory(outputHists);
    auto occPixel = new TH1D((sensor->getName() + "-occupancy_pixel").c_str(), "", 100, 0, 1);
    occPixel->SetDirectory(outputHists);
    auto mask = new TH2C((sensor->getName() + "-masked_pixels").c_str(),
                         "",
                         sensor->getNumX(),
                         0,
                         sensor->getNumX(),
                         sensor->getNumY(),
                         0,
                         sensor->getNumY());
    mask->SetDirectory(outputHists);

    m_occMaps.push_back(occMap);
    m_occPixels.push_back(occPixel);
    m_masks.push_back(mask);
  }
}

void Analyzers::NoiseAnalyzer::analyze(uint64_t eventId,
                                       const Storage::Event& event)
{
  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    auto id = m_sensorIds[i];
    auto occMap = m_occMaps[i];
    auto plane = event.getPlane(id);

    for (size_t j = 0; j < plane->getNumHits(); ++j) {
      auto hit = plane->getHit(j);
      auto col = hit->getPixX();
      auto row = hit->getPixY();
      if (m_roi.isInside({col, row}))
          occMap->Fill(col, row);
    }
  }
  m_numEvents += 1;
}

void Analyzers::NoiseAnalyzer::finalize()
{
  Mechanics::NoiseMask newMask;

  for (size_t i = 0; i < m_sensorIds.size(); ++i) {
    auto id = m_sensorIds[i];
    auto occMap = m_occMaps[i];
    auto occPixel = m_occPixels[i];
    auto maskedPixels = m_masks[i];

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

    auto occAvg = occPixel->GetMean();
    auto occStd = occPixel->GetStdDev();
    for (auto icol = occMap->GetNbinsX(); 0 < icol; --icol) {
      for (auto irow = occMap->GetNbinsY(); 0 < irow; --irow) {
        auto occ = occMap->GetBinContent(icol, irow);

        // pixel occupancy is a number of stddevs above global average
        bool isAboveRelative = ((occAvg + m_maxSigmaAboveAvg * occStd) < occ);
        // pixel occupancy is above absolute limit
        bool isAboveAbsolute = (m_maxOccupancy < occ);

        if (isAboveRelative || isAboveAbsolute) {
            maskedPixels->SetBinContent(icol, irow, 1);
            // index of first data bin in ROOT histograms is 1
            newMask.maskPixel(id, icol - 1, irow - 1);
        }
      }
    }

    INFO("Noise sensor ", id, ":\n");
    INFO("  roi col: [", m_roi.axes[0].min, ", ", m_roi.axes[0].max, "]\n");
    INFO("  roi row: [", m_roi.axes[1].min, ", ", m_roi.axes[1].max, "]\n");
    INFO("  max occupancy: ", occMap->GetMaximum(), " hits/event\n");
  }

  std::cout << "Generated Noise Mask:\n";
  newMask.print(std::cout, "  ");

  if (!m_outputNoiseMask.empty())
      Utils::Config::writeConfig(newMask.toConfig(), m_outputNoiseMask);
}
