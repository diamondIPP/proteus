/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \created 2016-08
 *
 * This is a conversion of the standalone `NoiseScanLooper` into a
 * reusable analyzer.
 */

#ifndef __JU_NOISE_H__
#define __JU_NOISE_H__

#include <memory>
#include <vector>

#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>

#include "analyzer.h"
#include "utils/config.h"
#include "utils/definitions.h"
#include "utils/interval.h"

namespace Mechanics {
class Device;
}

namespace Analyzers {

class NoiseAnalyzer : public Analyzer {
public:
  static std::unique_ptr<NoiseAnalyzer> make(const toml::Value& cfg,
                                             const Mechanics::Device& device,
                                             const std::string& outputNoiseMask,
                                             TDirectory* outputHists);

  std::string name() const;
  void analyze(uint64_t eventId, const Storage::Event& event);
  void finalize();

private:
  using SensorRoi = Utils::Box<2,unsigned int,Utils::Endpoints::CLOSED>;

  NoiseAnalyzer(const std::vector<int>& sensorIds,
                double maxSigmaAboveAvg,
                double maxOccupancy,
                SensorRoi roi,
                const std::string& outputNoiseMask);

  void initialize(const Mechanics::Device& device, TDirectory* outputHists);

  std::vector<int> m_sensorIds;
  std::vector<TH2D*> m_occMaps;
  std::vector<TH1D*> m_occPixels;
  std::vector<TH2C*> m_masks;
  uint64_t m_numEvents;
  double m_maxSigmaAboveAvg, m_maxOccupancy;
  SensorRoi m_roi;
  std::string m_outputNoiseMask;
};

} // namespace Analyzers

#endif // __JU_NOISE_H__
