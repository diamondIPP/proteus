#ifndef PT_TRACK_H
#define PT_TRACK_H

#include <functional>
#include <iosfwd>
#include <map>
#include <string>

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
  /** Clusters indexed by the sensor. */
  using Clusters = std::map<Index, std::reference_wrapper<Cluster>>;

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

  /** Adds a cluster on the given sensor to the track.
   *
   * The cluster/track association is not fixed automatically here.
   */
  void addCluster(Index sensor, Cluster& cluster);
  size_t size() const { return m_clusters.size(); }
  const Clusters& clusters() const { return m_clusters; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  TrackState m_state;
  float m_chi2;
  int m_dof;
  Index m_index;
  Clusters m_clusters;

  friend class Event;
};

} // namespace Storage

#endif // PT_TRACK_H
