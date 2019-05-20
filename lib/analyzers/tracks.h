// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>

#include "loop/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;
class TH2D;

namespace proteus {

class Device;

class Tracks : public Analyzer {
public:
  /** Construct a tracks analyzer.
   *
   * \param numTracksMax    Upper limit for tracks/event distribution
   * \param reducedChi2Max  Upper limit for chi^2/d.o.f. distribution
   * \param slopeRangeStd   Slope range measured in standard deviations
   * \param bins            Number of histogram bins
   */
  Tracks(TDirectory* dir,
         const Device& device,
         /* Histogram options */
         const int numTracksMax = 16,
         const double reducedChi2Max = 10,
         const double slopeStdRange = 5.0,
         const int bins = 128);

  std::string name() const;
  void execute(const Event& event);

  /** Average number of tracks per event. */
  double avgNumTracks() const;
  /** The beam slope (mean track slope) in global coordinates. */
  Vector2 beamSlope() const;
  /** The beam divergence (track slope standard dev) in global coordinates. */
  Vector2 beamDivergence() const;

private:
  TH1D* m_nTracks;
  TH1D* m_size;
  TH1D* m_reducedChi2;
  TH1D* m_prob;
  TH1D* m_posX;
  TH1D* m_posY;
  TH2D* m_posXY;
  TH1D* m_time;
  TH1D* m_slopeX;
  TH1D* m_slopeY;
  TH2D* m_slopeXY;
};

} // namespace proteus
