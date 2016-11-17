/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_TRACKFITTER_H
#define PT_TRACKFITTER_H

#include <vector>

#include "processors/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
class Sensor;
}

namespace Processors {

/** Estimate local track parameters using a straight line track model.
 *
 * This will fit the local track parameters on the selected sensor planes
 * and add them to the track as local parameters.
 */
class StraightTrackFitter : public Processor {
public:
  StraightTrackFitter(const Mechanics::Device& device,
                      const std::vector<Index>& sensorIds);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
  std::vector<Index> m_sensorIds;
};

} // namespace Processors

#endif // PT_TRACKFITTER_H
