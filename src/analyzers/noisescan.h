/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \created 2016-08
 *
 * This is a conversion of the standalone `NoiseScanLooper` into a
 * reusable analyzer.
 */

#ifndef PT_NOISESCAN_H
#define PT_NOISESCAN_H

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

class NoiseScan : public Analyzer {
public:
  NoiseScan(const Mechanics::Device& device,
                const toml::Value& cfg,
                TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  void writeMask(const std::string& path);

private:
  using Area = Utils::Box<2, int, Utils::Endpoints::CLOSED>;

  std::vector<int> m_sensorIds;
  std::vector<Area> m_sensorRois;
  std::vector<TH2D*> m_occMaps;
  std::vector<TH1D*> m_occPixels;
  std::vector<TH2D*> m_densities;
  std::vector<TH2D*> m_pixelMasks;
  uint64_t m_numEvents;
  double m_maxSigmaAboveAvg, m_maxOccupancy;
};

} // namespace Analyzers

#endif // PT_NOISESCAN_H
