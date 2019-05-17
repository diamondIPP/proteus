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

namespace proteus {

class Device;

/** Estimate local track parameters using a straight line track model.
 *
 * This calculates new global track parameters and goodness-of-fit and
 * calculates the local track parameters on the selected sensor planes.
 */
class GblFitter : public Processor {
public:
  GblFitter(const Device& device);

  std::string name() const;
  void execute(Event& event) const;

private:
  const Device& m_device;
  std::vector<Index> m_propagationIds;
};

} // namespace proteus

#endif // PT_GBLFITTER_H
