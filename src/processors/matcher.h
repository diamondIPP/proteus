#ifndef PT_MATCHER_H
#define PT_MATCHER_H

#include <string>

#include "processors/processor.h"
#include "utils/definitions.h"

namespace Storage {
class Plane;
class Cluster;
class Track;
}
namespace Mechanics {
class Device;
class Sensor;
}

namespace Processors {

/** Match tracks and clusters on a sensor plane.
 *
 * This matches the closest track/cluster pair together. The track must have
 * a local state on the selected sensor to be considered for matching. The
 * matching is unique, i.e. every track and every cluster is matched at most
 * once.
 */
class Matcher : public Processor {
public:
  /**
   * \param device The device setup.
   * \param sensorId The sensor for which matching should be calculated.
   * \param distanceSigmaMax Maximum matching significance, negativ disables.
   */
  Matcher(const Mechanics::Device& device,
          Index sensorId,
          double distanceSigmaMax = -1);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  Index m_sensorId;
  double m_distSquaredMax;
  std::string m_name;
};

} // namespace Processors

#endif // PT_MATCHER_H
