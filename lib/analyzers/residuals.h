// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include <unordered_map>
#include <vector>

#include "loop/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;
class TH2D;

namespace proteus {

class Device;
class Cluster;
class Sensor;
class TrackState;

namespace detail {

struct SensorResidualHists {
  TH1D* resU;
  TH1D* resV;
  TH1D* resS;
  TH2D* resUV;
  TH1D* resDist;
  TH1D* resD2;
  TH2D* posUResU;
  TH2D* posUResV;
  TH2D* posVResU;
  TH2D* posVResV;
  TH2D* timeResU;
  TH2D* timeResV;
  TH2D* slopeUResU;
  TH2D* slopeUResV;
  TH2D* slopeVResU;
  TH2D* slopeVResV;

  SensorResidualHists() = default;
  SensorResidualHists(TDirectory* dir,
                      const Sensor& sensor,
                      double rangeStd,
                      int bins,
                      const std::string& name);

  void fill(const TrackState& state, const Cluster& cluster);
};

} // namespace detail

class Residuals : public Analyzer {
public:
  /** Construct a residual analyzer.
   *
   * \param dir       Where to create the output subdirectory
   * \param device    The device object
   * \param sensorIds Sensors for which residuals should be calculated
   * \param subdir    Name of the output subdirectory
   * \param rangeStd  Residual/ slope range in expected standard deviations
   * \param bins      Number of histogram bins
   */
  Residuals(TDirectory* dir,
            const Device& device,
            const std::vector<Index>& sensorIds,
            const std::string& subdir = std::string("residuals"),
            double rangeStd = 5.0,
            int bins = 127);

  std::string name() const;
  void execute(const Event& refEvent);

private:
  std::unordered_map<Index, detail::SensorResidualHists> m_hists_map;
};

class Matching : public Analyzer {
public:
  /** Construct a matching analyzer.
   *
   * \param dir       Where to create the output subdirectory
   * \param device    The device object
   * \param sensorIds Sensors for which residuals should be calculated
   * \param rangeStd  Residual/ slope range in expected standard deviations
   * \param bins      Number of histogram bins
   */
  Matching(TDirectory* dir,
           const Sensor& sensor,
           double rangeStd = 8.0,
           int bins = 255);

  std::string name() const;
  void execute(const Event& refEvent);

private:
  Index m_sensorId;
  detail::SensorResidualHists m_hists;
};

} // namespace proteus
