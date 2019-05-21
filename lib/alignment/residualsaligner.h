// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>

#include "aligner.h"

class TDirectory;
class TH1D;

namespace proteus {

class Device;

/** Sensor alignment in the local plane using track residual histograms. */
class ResidualsAligner : public Aligner {
public:
  /**
   * \param damping    Scale factor for raw corrections to avoid oscillations
   * \param pixelRange Offset histogram range in number of pixels
   * \param gammaRange Rotation histogram range in radian
   * \param bins       Number of histogram bins
   */
  ResidualsAligner(TDirectory* dir,
                   const Device& device,
                   const std::vector<Index>& alignIds,
                   const double damping = 1,
                   const double pixelRange = 1.0,
                   const double gammaRange = 0.1,
                   const int bins = 128);
  ~ResidualsAligner() = default;

  std::string name() const;
  void execute(const Event& event);

  Geometry updatedGeometry() const;

private:
  struct SensorHists {
    Index sensorId;
    TH1D* corrU;
    TH1D* corrV;
    TH1D* corrGamma;
  };

  std::vector<SensorHists> m_hists;
  const Device& m_device;
  double m_damping;
};

} // namespace proteus
