#ifndef PT_CLUSTER_H
#define PT_CLUSTER_H

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"
#include "utils/interval.h"

namespace Mechanics {
class Sensor;
}

namespace Storage {

class Hit;

class Cluster {
public:
  using Area = Utils::Box<2, int>;
  using Hits = std::vector<std::reference_wrapper<Hit>>;

  Cluster(Index index);

  void setPixel(const XYPoint& cr, const SymMatrix2& cov);
  void setPixel(float col, float row, float stdCol, float stdRow);
  void setTime(float time_) { m_time = time_; }
  void setValue(float value_) { m_value = value_; }
  /** Calculate local and global coordinates from the pixel coordinates. */
  void transform(const Mechanics::Sensor& sensor);
  void setTrack(Index track);

  Index region() const;
  const XYPoint& posPixel() const { return m_cr; }
  const XYPoint& posLocal() const { return m_uv; }
  const XYZPoint& posGlobal() const { return m_xyz; }
  const SymMatrix2& covPixel() const { return m_crCov; }
  const SymMatrix2& covLocal() const { return m_uvCov; }
  const SymMatrix3& covGlobal() const { return m_xyzCov; }
  float time() const { return m_time; }
  float value() const { return m_value; }
  /** The area enclosing the cluster in pixel coordinates.
   *
   * \returns Empty area for an empty cluster.
   */
  Area areaPixel() const;
  int sizeCol() const;
  int sizeRow() const;

  void addHit(Hit& hit);
  size_t size() const { return m_hits.size(); }
  const Hits& hits() const { return m_hits; }

  Index index() const { return m_index; }
  bool isInTrack() const { return m_track != kInvalidIndex; }
  Index track() const { return m_track; }
  bool isMatched() const { return m_matchedState != kInvalidIndex; }
  Index matchedState() const { return m_matchedState; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  XYPoint m_cr;
  float m_time;  // The timing of the underlying hits
  float m_value; // The combined value of all its hits
  XYPoint m_uv;
  SymMatrix2 m_uvCov;
  SymMatrix2 m_crCov;
  XYZPoint m_xyz;
  SymMatrix3 m_xyzCov;

  Hits m_hits; // List of hits composing the cluster

  Index m_index;
  Index m_track;
  Index m_matchedState;

  friend class SensorEvent;
};

std::ostream& operator<<(std::ostream& os, const Cluster& cluster);

} // namespace Storage

#endif // PT_CLUSTER_H
