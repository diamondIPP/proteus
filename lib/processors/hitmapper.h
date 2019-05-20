/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#pragma once

#include "loop/processor.h"
#include "utils/definitions.h"

namespace proteus {

/** Map FE-I4 digital address to correct CCPDv4 sensor pixel address. */
class CCPDv4HitMapper : public Processor {
public:
  CCPDv4HitMapper(Index sensorId);

  std::string name() const;
  void execute(Event& event) const;

private:
  Index m_sensorId;
};

} // namespace proteus
