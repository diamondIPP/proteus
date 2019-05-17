/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2019-03
 */

#ifndef PT_SETUPFITTER_H
#define PT_SETUPFITTER_H

#include <string>

namespace proteus {

class Device;
class EventLoop;

/** Select a track fitter implementation by name. */
void setupTrackFitter(const Device& device,
                      const std::string& type,
                      EventLoop& loop);

} // namespace proteus

#endif // PT_SETUPFITTER_H
