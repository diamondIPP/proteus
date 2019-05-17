/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-11-09
 */

#ifndef PT_MATCHEXPORTER_H
#define PT_MATCHEXPORTER_H

#include <cstdint>
#include <string>

#include "loop/writer.h"
#include "utils/definitions.h"

class TDirectory;
class TTree;

namespace proteus {

class Cluster;
class Event;
class Track;
class TrackState;
class Sensor;
class SensorEvent;

/** Export matched (and unmatched) tracks and clusters to a TTree. */
class MatchWriter : public Writer {
public:
  MatchWriter(TDirectory* dir, const Sensor& sensor);

  std::string name() const override final;
  void append(const Event& event) override final;

private:
  static constexpr size_t kMaxClusterSize = 1024;

  struct EventData {
    uint64_t frame;
    uint64_t timestamp;
    int16_t nClusters;
    int16_t nTracks;

    void addToTree(TTree* tree);
    void set(const SensorEvent& e);
  };
  struct TrackData {
    float u, v, time, du, dv, dtime;
    float stdU, stdV, stdTime;
    float corrUV;
    float col, row, timestamp;
    float prob;
    float chi2;
    int16_t dof;
    int16_t size;

    void addToTree(TTree* tree);
    void
    set(const Track& track, const TrackState& state, const Vector4& posPixel);
  };
  struct ClusterData {
    float u, v, time;
    float stdU, stdV, stdTime;
    float corrUV;
    float col, row, timestamp, value;
    int16_t region;
    int16_t size, sizeCol, sizeRow;
    int16_t hitCol[kMaxClusterSize];
    int16_t hitRow[kMaxClusterSize];
    int16_t hitTimestamp[kMaxClusterSize];
    int16_t hitValue[kMaxClusterSize];

    void addToTree(TTree* tree);
    void set(const Cluster& cluster);
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

  const Sensor& m_sensor;
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

} // namespace proteus

#endif // PT_NOISESCAN_H
