/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-11-09
 */

#ifndef PT_MATCHEXPORTER_H
#define PT_MATCHEXPORTER_H

#include <cstdint>
#include <string>

#include "analyzers/analyzer.h"
#include "utils/definitions.h"

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
  static constexpr int16_t MAX_CLUSTER_SIZE = 1024;

  struct EventData {
    uint64_t timestamp;
    int16_t nClusters;
    int16_t nTracks;
  };
  struct TrackData {
    float u, v, du, dv;
    float stdU, stdV, corrUV;
    float col, row;
    float chi2;
    int16_t dof;
    int16_t nClusters;
  };
  struct ClusterData {
    float u, v;
    float stdU, stdV, corrUV;
    float col, row;
    float time, value;
    int16_t region;
    int16_t size, sizeCol, sizeRow;
    int16_t hitCol[MAX_CLUSTER_SIZE];
    int16_t hitRow[MAX_CLUSTER_SIZE];
    float hitTime[MAX_CLUSTER_SIZE];
    float hitValue[MAX_CLUSTER_SIZE];
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
  std::string m_name;
};

} // namespace Analyzers

#endif // PT_NOISESCAN_H
