#ifndef PT_TRACK_H
#define PT_TRACK_H

#include <cmath>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace Tracking {
class TrackFinder;
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

  void setGoodnessOfFit(double chi2, int dof) { m_chi2 = chi2, m_dof = dof; }
  double chi2() const { return m_chi2; };
  double reducedChi2() const { return m_chi2 / m_dof; }
  double degreesOfFreedom() const { return m_dof; }

  void setGlobalState(const TrackState& state) { m_state = state; }
  const TrackState& globalState() const { return m_state; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  /** Inform all track clusters that they belong to this track now. */
  void freezeClusterAssociation();

  TrackState m_state;
  double m_chi2;
  int m_dof;
  Index m_index;

  std::vector<Cluster*> m_clusters;

  friend class Event;
  friend class Tracking::TrackFinder;
};

} // namespace Storage

#endif // PT_TRACK_H
