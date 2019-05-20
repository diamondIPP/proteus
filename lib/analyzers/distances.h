// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-12
 */

#pragma once

#include <string>

#include "loop/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;

namespace proteus {

class Sensor;

class Distances : public Analyzer {
public:
  Distances(TDirectory* dir, const Sensor& sensor);

  std::string name() const;
  void execute(const Event& event);

private:
  struct Hists {
    TH1D* deltaU = nullptr;
    TH1D* deltaV = nullptr;
    TH1D* deltaS = nullptr;
    TH1D* dist = nullptr;
  };

  Index m_sensorId = kInvalidIndex;
  Hists m_trackTrack;
  Hists m_trackCluster;
  Hists m_clusterCluster;
};

} // namespace proteus
