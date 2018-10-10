#ifndef PT_APPLYGEOMETRY_H
#define PT_APPLYGEOMETRY_H

#include "loop/processor.h"

namespace Mechanics {
class Device;
}

namespace Processors {

/** Use device geometry to set global positions for clusters.
 *
 * \warning This does **not** update existing track parameters. This must be
 *          updated by refitting the track.
 */
class ApplyGeometry : public Loop::Processor {
public:
  ApplyGeometry(const Mechanics::Device& device);

  std::string name() const;
  void execute(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
};

} // namespace Processors

#endif // PT_APPLYGEOMETRY_H
