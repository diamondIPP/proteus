#ifndef PT_TRACK_H
#define PT_TRACK_H

#include <iosfwd>
#include <vector>

#include "storage/trackstate.h"
#include "utils/definitions.h"

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

  void setGoodnessOfFit(float chi2, int dof) { m_chi2 = chi2, m_dof = dof; }
  void setGlobalState(const TrackState& state) { m_state = state; }
  /** Inform all track clusters that they belong to this track now. */
  void freezeClusterAssociation();

  float chi2() const { return m_chi2; }
  float reducedChi2() const { return m_chi2 / m_dof; }
  float degreesOfFreedom() const { return m_dof; }
  const TrackState& globalState() const { return m_state; }

  /** Adds the cluster to the track but does not inform the cluster about it. */
  void addCluster(Cluster* cluster);
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i); }

  Index index() const { return m_index; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  TrackState m_state;
  float m_chi2;
  int m_dof;
  Index m_index;

  std::vector<Cluster*> m_clusters;

  friend class Event;
};

} // namespace Storage

#endif // PT_TRACK_H
