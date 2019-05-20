#pragma once

#include <map>

#include <TH1D.h>

#include "loop/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH2D;

namespace proteus {

class Device;
class Geometry;
class Sensor;

class Correlations : public Analyzer {
public:
  /** Consider pair-wise correlations between neighboring sensors.
   *
   * \param device     The telescope device.
   * \param sensorIds  Correlations are calculate for these sensors in order.
   * \param dir        Where to put the output histograms.
   * \param neighbors  How many neighboring planes to consider; must be > 1.
   */
  Correlations(TDirectory* dir,
               const Device& device,
               const std::vector<Index>& sensorIds,
               const int neighbors = 2);
  /** Consider pair-wise correlations between all configured sensors.
   *
   * \param device     The telescope device.
   * \param neighbors  How many neighboring planes to consider; must be > 1.
   *
   * Correlations are calculated for all configured sensors ordered along the
   * beam direction.
   */
  Correlations(TDirectory* dir, const Device& device, const int neighbors = 2);

  std::string name() const;
  void execute(const Event& event);

  const TH1D* getHistDiffX(Index sensorId0, Index sensorId1) const;
  const TH1D* getHistDiffY(Index sensorId0, Index sensorId1) const;

private:
  // Shared function to initialize the correlation hist between two sensors
  void addHist(const Sensor& sensor0, const Sensor& sensor1, TDirectory* dir);

  struct Hists {
    TH2D* corrX = nullptr;
    TH2D* corrY = nullptr;
    TH2D* corrT = nullptr;
    TH1D* diffX = nullptr;
    TH1D* diffY = nullptr;
    TH1D* diffT = nullptr;
  };

  const Geometry& m_geo;
  std::map<std::pair<Index, Index>, Hists> m_hists;
};

} // namespace proteus
