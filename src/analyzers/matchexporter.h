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
} // namespace Mechanics
namespace Storage {
class Cluster;
class Event;
class Plane;
class Track;
class TrackState;
} // namespace Storage

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
    uint64_t frame;
    uint64_t timestamp;
    int16_t nClusters;
    int16_t nTracks;

    void addToTree(TTree* tree);
    void set(const Storage::Event& e, const Storage::Plane& s);
  };
  struct TrackData {
    float u, v, du, dv;
    float stdU, stdV, corrUV;
    float col, row;
    float chi2;
    int16_t dof;
    int16_t size;

    void addToTree(TTree* tree);
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

    void addToTree(TTree* tree);
    void set(const Storage::Cluster& c);
    void invalidate();
  };
  struct DistData {
    float d2;

    void addToTree(TTree* tree);
    void invalidate();
  };
  struct MaskData {
    int16_t col, row;

    void addToTree(TTree* tree);
  };

  const Mechanics::Sensor& m_sensor;
  Index m_sensorId;
  EventData m_event;
  TrackData m_track;
  ClusterData m_matchedCluster;
  ClusterData m_unmatchCluster;
  DistData m_matchedDist;
  TTree* m_matchedTree;
  TTree* m_unmatchTree;
  std::string m_name;
};

} // namespace Analyzers

#endif // PT_NOISESCAN_H
