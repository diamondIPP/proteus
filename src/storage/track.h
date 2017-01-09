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
class TrackFinder;
class TrackMaker;
}

namespace Storage {

class Cluster;

/** A particle track.
 *
 * The track consist of a set of input clusters, global track, and
 * goodness-of-fit information.
 */
class Track {
public:
  Track();
  Track(const TrackState& global);

  Index index() const { return m_index; }

  /** Adds the cluster to the track but does not inform the cluster about it. */
  void addCluster(Cluster* cluster);
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i); }

  void setGoodnessOfFit(double chi2, int ndf) { m_redChi2 = chi2 / ndf; }
  void setGoodnessOfFit(double reducedChi2) { m_redChi2 = reducedChi2; }
  double reducedChi2() const { return m_redChi2; }

  void setGlobalState(const TrackState& state) { m_state = state; }
  const TrackState& globalState() const { return m_state; }

  /* \deprecated Use TrackState for matching information. */
  void setMatchedCluster(Index sensorId, const Cluster* cluster);
  const Cluster* getMatchedCluster(Index sensorId) const;

  unsigned int getNumClusters() const { return m_clusters.size(); }
  double getOriginX() const { return m_state.offset().x(); }
  double getOriginY() const { return m_state.offset().y(); }
  double getOriginErrX() const { return std::sqrt(m_state.m_cov(0, 0)); }
  double getOriginErrY() const { return std::sqrt(m_state.m_cov(1, 1)); }
  double getSlopeX() const { return m_state.slope().x(); }
  double getSlopeY() const { return m_state.slope().y(); }
  double getSlopeErrX() const { return std::sqrt(m_state.m_cov(2, 2)); }
  double getSlopeErrY() const { return std::sqrt(m_state.m_cov(3, 3)); }
  double getCovarianceX() const { return m_state.m_cov(0, 2); }
  double getCovarianceY() const { return m_state.m_cov(1, 3); }
  double getChi2() const { return m_redChi2; }
  int getIndex() const { return m_index; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  /** Inform all track clusters that they belong to this track now. */
  void freezeClusterAssociation();

  TrackState m_state;
  double m_redChi2;
  Index m_index;

  std::vector<Cluster*> m_clusters;
  // NOTE: this isn't stored in file, its a place holder for doing DUT analysis
  std::vector<const Cluster*> m_matchedClusters;

  friend class Event;
  friend class Processors::TrackFinder;
  friend class Processors::TrackMaker;
};

} // namespace Storage

#endif // PT_TRACK_H
