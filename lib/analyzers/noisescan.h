// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 *
 * This is a conversion of the standalone `NoiseScanLooper` into a
 * reusable analyzer.
 */

#pragma once

#include <vector>

#include "loop/analyzer.h"
#include "mechanics/pixelmasks.h"
#include "utils/definitions.h"
#include "utils/interval.h"

class TDirectory;
class TH1D;
class TH2D;

namespace proteus {
class Sensor;

/** Estimate noisy pixels from hit occupancies.
 *
 * Noise estimation uses a local estimate of the expected hit rate to
 * find pixels that are a certain number of standard deviations away from
 * this estimate.
 */
class NoiseScan : public Analyzer {
public:
  typedef Box<2, int> Area;

  NoiseScan(TDirectory* dir,
            const Sensor& sensor,
            const double bandwidth,
            const double sigmaMax,
            const double rateMax,
            const Area& regionOfInterest,
            const int binsOccupancy = 128);

  std::string name() const;
  void execute(const Event& event);
  void finalize();

  PixelMasks constructMasks() const;

private:
  Index m_sensorId;
  double m_bandwidthCol, m_bandwidthRow;
  double m_sigmaMax, m_rateMax;
  uint64_t m_numEvents;
  TH2D* m_occupancy;
  TH1D* m_occupancyDist;
  TH2D* m_density;
  TH2D* m_significance;
  TH1D* m_significanceDist;
  TH2D* m_maskAbsolute;
  TH2D* m_maskRelative;
  TH2D* m_mask;
};

} // namespace proteus
