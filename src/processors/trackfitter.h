/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_TRACKFITTER_H
#define PT_TRACKFITTER_H

#include <set>

#include "processors/processor.h"

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
  template <typename Container>
  StraightTrackFitter(const Mechanics::Device& device,
                      const Container& sensorIds)
      : m_device(device)
      , m_sensorIds(std::begin(sensorIds), std::end(sensorIds))
  {
  }

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
  std::set<int> m_sensorIds;
};

} // namespace Processors

#endif // PT_TRACKFITTER_H
