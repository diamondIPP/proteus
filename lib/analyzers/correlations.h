#ifndef PT_CORRELATION_H
#define PT_CORRELATION_H

#include <map>

#include <TH1D.h>

#include "loop/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH2D;

namespace Mechanics {
class Device;
class Geometry;
class Sensor;
} // namespace Mechanics

namespace Analyzers {

class Correlations : public Loop::Analyzer {
public:
  /** Consider pair-wise correlations between neighboring sensors.
   *
   * \param device     The telescope device.
   * \param sensorIds  Correlations are calculate for these sensors in order.
   * \param dir        Where to put the output histograms.
   * \param neighbors  How many neighboring planes to consider; must be > 1.
   */
  Correlations(TDirectory* dir,
               const Mechanics::Device& device,
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
  Correlations(TDirectory* dir,
               const Mechanics::Device& device,
               const int neighbors = 2);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  const TH1D* getHistDiffX(Index sensorId0, Index sensorId1) const;
  const TH1D* getHistDiffY(Index sensorId0, Index sensorId1) const;

private:
  // Shared function to initialize the correlation hist between two sensors
  void addHist(const Mechanics::Sensor& sensor0,
               const Mechanics::Sensor& sensor1,
               TDirectory* dir);

  typedef std::pair<Index, Index> Indices;
  struct Hists {
    TH2D* corrX;
    TH2D* corrY;
    TH1D* diffX;
    TH1D* diffY;
  };

  const Mechanics::Geometry& m_geo;
  std::map<Indices, Hists> m_hists;
};

} // namespace Analyzers

#endif // PT_CORRELATION_H
