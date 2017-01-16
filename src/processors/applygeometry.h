#ifndef PT_APPLYGEOMETRY_H
#define PT_APPLYGEOMETRY_H

#include "processors/processor.h"

namespace Mechanics {
class Device;
}

namespace Processors {

/** Use device geometry to set global positions for clusters.
 *
 * \warning This does **not** update existing track parameters. This must be
 *          updated by refitting the track.
 */
class ApplyGeometry : public Processor {
public:
  ApplyGeometry(const Mechanics::Device& device);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
};

/** \deprecated Use ApplyGeometry processor via EventLoop. */
void applyAlignment(Storage::Event* event, const Mechanics::Device* device);

} // namespace Processors

#endif // PT_APPLYGEOMETRY_H
