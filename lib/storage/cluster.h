#ifndef PT_CLUSTER_H
#define PT_CLUSTER_H

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"
#include "utils/interval.h"

namespace Storage {

class Hit;

class Cluster {
public:
  using Area = Utils::Box<2, int>;
  using Hits = std::vector<std::reference_wrapper<Hit>>;

  /** Construct a cluster using pixel coordinates. */
  Cluster(Scalar col,
          Scalar row,
          Scalar timestamp,
          Scalar value,
          Scalar colVar,
          Scalar rowVar,
          Scalar timestampVar,
          Scalar colRowCov = 0);

  template <typename Position, typename Covariance>
  void setLocal(const Eigen::MatrixBase<Position>& loc,
                const Eigen::MatrixBase<Covariance>& cov)
  {
    m_pos = loc;
    m_posCov = cov.template selfadjointView<Eigen::Lower>();
  }
  void setTrack(Index track);

  // properties in the pixel system
  Scalar col() const { return m_col; }
  Scalar colVar() const { return m_colVar; }
  Scalar row() const { return m_row; }
  Scalar rowVar() const { return m_rowVar; }
  Scalar colRowCov() const { return m_colRowCov; }
  Scalar timestamp() const { return m_timestamp; }
  Scalar timestampVar() const { return m_timestampVar; }
  Scalar value() const { return m_value; }

  // properties in the local system
  /** On-plane spatial u coordinate. */
  Scalar u() const { return m_pos[kU]; }
  /** On-plane spatial v coordinate. */
  Scalar v() const { return m_pos[kV]; }
  /** On-plane spatial covariance. */
  auto uvCov() const { return m_posCov.block<2, 2>(kU, kU); }
  /** Local time. */
  Scalar time() const { return m_pos[kS]; }
  /** Local time variance. */
  Scalar timeVar() const { return m_posCov(kS, kS); }
  /** Full position in local coordinates. */
  const Vector4& position() const { return m_pos; }
  /** Full position covariance in local coordinates. */
  const SymMatrix4& positionCov() const { return m_posCov; }

  /** The area enclosing the cluster in pixel coordinates.
   *
   * \returns Empty area for an empty cluster.
   */
  Area areaPixel() const;
  size_t sizeCol() const { return areaPixel().length(0); }
  size_t sizeRow() const { return areaPixel().length(1); }

  bool hasRegion() const;
  Index region() const;

  void addHit(Hit& hit);
  size_t size() const { return m_hits.size(); }
  const Hits& hits() const { return m_hits; }

  Index index() const { return m_index; }
  bool isInTrack() const { return m_track != kInvalidIndex; }
  Index track() const { return m_track; }
  bool isMatched() const { return m_matchedState != kInvalidIndex; }
  Index matchedState() const { return m_matchedState; }

private:
  Scalar m_col;
  Scalar m_row;
  Scalar m_timestamp;
  Scalar m_value;
  Scalar m_colVar;
  Scalar m_rowVar;
  Scalar m_colRowCov;
  Scalar m_timestampVar;
  Vector4 m_pos;
  SymMatrix4 m_posCov;

  Hits m_hits; // List of hits composing the cluster

  Index m_index;
  Index m_track;
  Index m_matchedState;

  friend class SensorEvent;
};

std::ostream& operator<<(std::ostream& os, const Cluster& cluster);

} // namespace Storage

#endif // PT_CLUSTER_H
