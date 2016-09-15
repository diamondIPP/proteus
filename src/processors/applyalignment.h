#ifndef PT_APPLYALIGNMENT_H
#define PT_APPLYALIGNMENT_H

#include "processor.h"

namespace Mechanics {
class Device;
}

namespace Processors {

void applyAlignment(Storage::Event* event, const Mechanics::Device* device);

class ApplyAlignment : public Processor {
public:
  ApplyAlignment(const Mechanics::Device& device);

  std::string name() const;
  void process(uint64_t eventId, Storage::Event& event);
  void finalize();

private:
  const Mechanics::Device& m_device;
};

} // namespace Processors

#endif // PT_APPLYALIGNMENT_H
