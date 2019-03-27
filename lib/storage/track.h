#ifndef PT_TRACK_H
#define PT_TRACK_H

#include <iosfwd>
#include <vector>

#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace Storage {

/** A particle track.
 *
 * The track consist of a set of input clusters, global track, and
 * goodness-of-fit information.
 */
class Track {
public:
  /** Clusters indexed by the sensor. */
  struct TrackCluster {
    Index sensor;
    Index cluster;
  };

  /** Construct a track without hits and undefined global state. */
  Track() : m_chi2(-1), m_dof(-1) {}
  /** Construct a track without hits but with known global state. */
  Track(const TrackState& global, Scalar chi2 = -1, int dof = -1);

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
   * This enforces a single-cluster-per-sensor rule i.e. if another cluster
   * was previously added for the same sensor than it will be replaced with
   * the new one.
   */
  void addCluster(Index sensor, Index cluster);
  /** The size of the track, i.e. the number of associated clusters. */
  size_t size() const { return m_clusters.size(); }
  /** Check if the track contains a cluster on the given sensor. */
  bool hasClusterOn(Index sensor) const;
  /** Get the cluster on the requested sensor. */
  Index getClusterOn(Index sensor) const;
  /** Get the list of all associated clusters. */
  const std::vector<TrackCluster>& clusters() const { return m_clusters; }

private:
  TrackState m_state;
  Scalar m_chi2;
  int m_dof;
  std::vector<TrackCluster> m_clusters;

  friend class Event;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

std::ostream& operator<<(std::ostream& os, const Track& track);

} // namespace Storage

#endif // PT_TRACK_H
