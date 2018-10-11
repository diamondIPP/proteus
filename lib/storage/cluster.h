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

  Cluster();

  void setPixel(const Vector2& cr, const SymMatrix2& cov);
  void setLocal(const Vector2& uv, const SymMatrix2& cov);
  void setTime(float time_) { m_time = time_; }
  void setValue(float value_) { m_value = value_; }
  void setTrack(Index track);

  Vector2 posPixel() const { return {m_col, m_row}; }
  Vector2 posLocal() const { return {m_u, m_v}; }
  auto covPixel() const { return m_crCov; }
  auto covLocal() const { return m_uvCov; }
  float time() const { return m_time; }
  float value() const { return m_value; }
  /** The area enclosing the cluster in pixel coordinates.
   *
   * \returns Empty area for an empty cluster.
   */
  Area areaPixel() const;
  int sizeCol() const;
  int sizeRow() const;

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

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  float m_col, m_row;
  float m_u, m_v;
  float m_time;
  float m_value;
  SymMatrix2 m_crCov;
  SymMatrix2 m_uvCov;

  Hits m_hits; // List of hits composing the cluster

  Index m_index;
  Index m_track;
  Index m_matchedState;

  friend class SensorEvent;
};

std::ostream& operator<<(std::ostream& os, const Cluster& cluster);

} // namespace Storage

#endif // PT_CLUSTER_H
