// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02-16
 */

#pragma once

#include <vector>

#include "loop/analyzer.h"
#include "utils/definitions.h"
#include "utils/densemask.h"
#include "utils/interval.h"

class TDirectory;
class TH1D;
class TH2D;

namespace proteus {

class Cluster;
class Sensor;
class TrackState;

/** Efficiency calculation using tracks and matched clusters.
 *
 * Computes sensor and in-pixel efficiency maps and projections. Two-dimensional
 * efficiency maps are calculated with additional edges to also include tracks
 * that are matched to a cluster but are located outside the region-of-interest.
 * The per-pixel efficiency distribution is calculated without these edges
 * pixels.
 *
 * For the column and row projections, tracks are considered only if they fall
 * within the region-of-interest in the other axis. E.g. the column projections
 * are calculated only for tracks whose row position falls within the
 * region-of-interest excluding the additional edges.
 *
 * The in-pixel efficiencies are only calculated for tracks fully within the
 * region-of-interest excluding the additional edges.
 */
class Efficiency : public Analyzer {
public:
  /**
   * \param sensor Sensor for which efficiencies should be calculated
   * \param dir Histogram output directory
   * \param maskedPixelRange Remove tracks around masked pixels, 0 to disable
   * \param increaseArea Extend histograms beyond the nominal sensor edge
   * \param inPixelPeriod Folding period in number of pixels
   * \param inPixelBinsMin Minimum number of bins along the smaller direction
   * \param efficiencyDistBins Number of bins in the efficiency distribution
   */
  Efficiency(TDirectory* dir,
             const Sensor& sensor,
             const int maskedPixelRange = 1,
             const int increaseArea = 0,
             const int inPixelPeriod = 2,
             const int inPixelBinsMin = 32,
             const int efficiencyDistBins = 128);

  std::string name() const;
  void execute(const Event& event);
  void finalize();

private:
  using DigitalArea = Box<2, int>;
  using Area = Box<2, Scalar>;
  struct Hists {
    DigitalArea areaPixel; // region-of-interest area + edge bins
    DigitalArea roiPixel;  // only the region-of-interest
    int edgeBins; // how many bins are edges outside the region-of-interest
    TH2D* total;
    TH2D* pass;
    TH2D* fail;
    TH2D* eff;
    TH1D* effDist;
    TH1D* colTotal;
    TH1D* colPass;
    TH1D* colFail;
    TH1D* colEff;
    TH1D* rowTotal;
    TH1D* rowPass;
    TH1D* rowFail;
    TH1D* rowEff;
    Area inPixelAreaLocal; // in local coordinates
    TH2D* inPixTotal;
    TH2D* inPixPass;
    TH2D* inPixFail;
    TH2D* inPixEff;
    TH2D* clustersPass;
    TH2D* clustersFail;

    Hists() = default;
    Hists(TDirectory* dir,
          const Sensor& sensor,
          const DigitalArea& roi,
          const int increaseArea,
          const int inPixelPeriod,
          const int inPixelBinsMin,
          const int efficiencyDistBins);
    void fill(const TrackState& state, Scalar col, Scalar row);
    void fill(const Cluster& cluster);
    void finalize();
  };

  const Sensor& m_sensor;
  DenseMask m_mask;
  Hists m_sensorHists;
  std::vector<Hists> m_regionsHists;
};

} // namespace proteus
