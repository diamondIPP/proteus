/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#ifndef PT_HITMAPPER_H
#define PT_HITMAPPER_H

#include "processors/processor.h"
#include "utils/definitions.h"

namespace Processors {

/** Map FE-I4 digital address to correct CCPDv4 sensor pixel address. */
class CCPDv4HitMapper : public Processor {
public:
  CCPDv4HitMapper(Index sensorId);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  Index m_sensorId;
};

} // namespace Processors

#endif // PT_HITMAPPER_H
