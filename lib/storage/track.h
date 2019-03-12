#ifndef PT_TRACK_H
#define PT_TRACK_H

#include <functional>
#include <iosfwd>
#include <map>

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

  /** Construct a track without hits and undefined global state. */
  Track();
  /** Construct a track without hits but with known global parameters. */
  Track(const TrackState& global);

  /** Update the goodness-of-fit via χ² and degrees-of-freedom. */
  void setGoodnessOfFit(Scalar chi2, int dof) { m_chi2 = chi2, m_dof = dof; }
  Scalar chi2() const { return m_chi2; }
  Scalar reducedChi2() const { return m_chi2 / m_dof; }
  int degreesOfFreedom() const { return m_dof; }
  /** Track fit probability.
   *
   * This is computed as 1 - CDF_{df}(χ²), i.e. assuming a χ² distribution
   * with df degrees of freedom. A small value close to 0 corresponds to a
   * bad fit with large residuals while a large value close to 1 corresponds
   * to a good fit with smaller residuals.
   */
  Scalar probability() const;

  /** Update the global track state. */
  template <typename... Params>
  void setGlobalState(Params&&... params)
  {
    m_state = TrackState(std::forward<Params>(params)...);
  }
  const TrackState& globalState() const { return m_state; }

  /** Adds a cluster on the given sensor to the track.
   *
   * The cluster/track association is not fixed automatically here.
   */
  void addCluster(Index sensor, Cluster& cluster);
  size_t size() const { return m_clusters.size(); }
  const Clusters& clusters() const { return m_clusters; }

private:
  /** Inform all track clusters that they belong to this track now. */
  void freezeClusterAssociation();

  TrackState m_state;
  Scalar m_chi2;
  int m_dof;
  Index m_index;
  Clusters m_clusters;

  friend class Event;
};

std::ostream& operator<<(std::ostream& os, const Track& track);

} // namespace Storage

#endif // PT_TRACK_H
