/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2019-03
 */

#ifndef PT_SETUPFITTER_H
#define PT_SETUPFITTER_H

#include <string>

namespace Mechanics {
class Device;
}
namespace Loop {
class EventLoop;
}
namespace Tracking {

/** Select a track fitter implementation by name. */
void setupTrackFitter(const Mechanics::Device& device,
                      const std::string& type,
                      Loop::EventLoop& loop);

} // namespace Tracking

#endif // PT_SETUPFITTER_H
