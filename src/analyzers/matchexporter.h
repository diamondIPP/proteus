/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-11-09
 */

#ifndef PT_MATCHEXPORTER_H
#define PT_MATCHEXPORTER_H

#include <string>

#include "analyzers/analyzer.h"
#include "utils/definitions.h"
#include "utils/statistics.h"

class TDirectory;
class TTree;

namespace Mechanics {
class Device;
class Sensor;
}

namespace Analyzers {

/** Export matched (and unmatched) tracks and clusters to a  TTree. */
class MatchExporter : public Analyzer {
public:
  MatchExporter(const Mechanics::Device& device,
                Index sensorId,
                TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  struct EventData {
    int nTracks;
  };
  struct TrackData {
    float u, v;
    float stdU, stdV, corrUV;
    float col, row;
    float chi2;
    int dof;
    int nClusters;
  };
  struct ClusterData {
    float u, v;
    float stdU, stdV, corrUV;
    float col, row;
    int size, sizeCol, sizeRow;
  };
  struct MatchData {
    float d2;
  };

  const Mechanics::Sensor& m_sensor;
  Index m_sensorId;
  EventData m_event;
  TrackData m_track;
  MatchData m_match;
  ClusterData m_clusterMatched;
  ClusterData m_clusterUnmatched;
  TTree* m_treeTrk;
  TTree* m_treeClu;
  Utils::StatAccumulator<double> m_statMatTrkFraction;
  Utils::StatAccumulator<double> m_statMatCluFraction;
  Utils::StatAccumulator<double> m_statUnmTrk;
  Utils::StatAccumulator<double> m_statUnmClu;
  std::string m_name;
};

} // namespace Analyzers

#endif // PT_NOISESCAN_H
