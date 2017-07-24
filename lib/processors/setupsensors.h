/** Automated setup of per-sensor processors.
 *
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#ifndef PT_SETUPSENSORS_H
#define PT_SETUPSENSORS_H

namespace Mechanics {
class Device;
}
namespace Utils {
class EventLoop;
}

namespace Processors {

/** Add hit mapper and region selection processors to the event loop. */
void setupHitPreprocessing(const Mechanics::Device& device,
                           Utils::EventLoop& loop);

/** Select and add clusterizer for all configured sensors. */
void setupClusterizers(const Mechanics::Device& device, Utils::EventLoop& loop);

} // namespace Processors

#endif // PT_SETUPSENSORS_H
