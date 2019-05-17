#ifndef PT_MATCHER_H
#define PT_MATCHER_H

#include <string>

#include "loop/processor.h"
#include "utils/definitions.h"

namespace proteus {

class Device;

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
  Matcher(const Device& device, Index sensorId, double distanceSigmaMax = -1);

  std::string name() const;
  void execute(Event& event) const;

private:
  Index m_sensorId;
  double m_distSquaredMax;
  std::string m_name;
};

} // namespace proteus

#endif // PT_MATCHER_H
