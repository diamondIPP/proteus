/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_GBLFITTER_H
#define PT_GBLFITTER_H

#include <vector>

#include "loop/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
} // namespace Mechanics
namespace Tracking {

/** Estimate local track parameters using a straight line track model.
 *
 * This calculates new global track parameters and goodness-of-fit and
 * calculates the local track parameters on the selected sensor planes.
 */
class GblFitter : public Loop::Processor {
public:
  GblFitter(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
  std::vector<Index> m_propagationIds;
};

} // namespace Tracking

#endif // PT_GBLFITTER_H