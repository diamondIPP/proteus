/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 *
 * This is a conversion of the standalone `NoiseScanLooper` into a
 * reusable analyzer.
 */

#ifndef PT_NOISESCAN_H
#define PT_NOISESCAN_H

#include <vector>

#include "analyzer.h"
#include "mechanics/pixelmasks.h"
#include "utils/definitions.h"
#include "utils/interval.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
}

namespace Analyzers {

/** Estimate noisy pixels from hit occupancies.
 *
 * Noise estimation uses a local estimate of the expected hit rate to
 * find pixels that are a certain number of standard deviations away from
 * this estimate.
 */
class NoiseScan : public Analyzer {
public:
  typedef Utils::Box<2, int> Area;

  NoiseScan(const Mechanics::Device& device,
            const Index sensorId,
            const double bandwidth,
            const double sigmaMax,
            const double rateMax,
            const Area& regionOfInterest,
            TDirectory* dir,
            const int binsOccupancy = 128);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  Mechanics::PixelMasks constructMasks() const;

private:
  Index m_sensorId;
  double m_densityBandwidth, m_sigmaMax, m_rateMax;
  Area m_roi;
  uint64_t m_numEvents;
  TH2D* m_occ;
  TH1D* m_occPixels;
  TH2D* m_density;
  TH2D* m_sigma;
  TH1D* m_sigmaPixels;
  TH2D* m_maskedPixels;
};

} // namespace Analyzers

#endif // PT_NOISESCAN_H
