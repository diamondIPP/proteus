/** Automated setup of per-sensor processors.
 *
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02
 */

#pragma once

namespace proteus {

class Device;
class EventLoop;

/** Add hit mapper and region selection processors to the event loop. */
void setupHitPreprocessing(const Device& device, EventLoop& loop);

/** Select and add clusterizer for all configured sensors. */
void setupClusterizers(const Device& device, EventLoop& loop);

} // namespace proteus
