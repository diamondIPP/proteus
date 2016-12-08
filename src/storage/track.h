#ifndef PT_TRACK_H
#define PT_TRACK_H

#include <cmath>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace Processors {
class TrackMaker;
}

namespace Storage {

class Cluster;

/** A particle track.
 *
 * The track consist of a set of input clusters and a set of reconstructed
 * track states on selected sensorId planes.
 */
class Track {
public:
  Track();
  Track(const TrackState& globalState);

  /** Adds the cluster to the track but does not inform the cluster about it. */
  void addCluster(Cluster* cluster);
  /** Inform all track clusters that they belong to this track now. */
  void fixClusterAssociation();
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i); }

  // global track information.
  void setGlobalState(const TrackState& state) { m_state = state; }
  void setGoodnessOfFit(double chi2, int ndf) { m_redChi2 = chi2 / ndf; }
  void setGoodnessOfFit(double reducedChi2) { m_redChi2 = reducedChi2; }
  const TrackState& globalState() const { return m_state; }
  double reducedChi2() const { return m_redChi2; }

  // local state information.
  void setLocalState(Index sensorId, const TrackState& state);
  bool hasLocalState(Index sensorId) const;
  const TrackState& getLocalState(Index sensorId) const;

  // matched cluster information. only used transiently, not stored.
  void setMatchedCluster(Index sensorId, const Cluster* cluster);
  bool hasMatchedCluster(Index sensorId) const;
  const Cluster* getMatchedCluster(Index sensorId) const;

  unsigned int getNumClusters() const { return m_clusters.size(); }
  double getOriginX() const { return m_state.offset().x(); }
  double getOriginY() const { return m_state.offset().y(); }
  double getOriginErrX() const { return m_state.stdOffsetU(); }
  double getOriginErrY() const { return m_state.stdOffsetV(); }
  double getSlopeX() const { return m_state.slope().x(); }
  double getSlopeY() const { return m_state.slope().y(); }
  double getSlopeErrX() const { return m_state.stdSlopeU(); }
  double getSlopeErrY() const { return m_state.stdSlopeV(); }
  double getCovarianceX() const { return m_state.m_cov(0, 2); }
  double getCovarianceY() const { return m_state.m_cov(1, 3); }
  double getChi2() const { return m_redChi2; }
  int getIndex() const { return m_index; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  TrackState m_state;
  std::map<Index, TrackState> m_localStates;
  double m_redChi2;
  int m_index;

  std::vector<Cluster*> m_clusters;
  // NOTE: this isn't stored in file, its a place holder for doing DUT analysis
  std::vector<Cluster*> m_matchedClusters;

  std::map<Index, const Cluster*> m_matches;

  friend class Event;
  friend class Processors::TrackMaker;
};

} // namespace Storage

#endif // PT_TRACK_H
