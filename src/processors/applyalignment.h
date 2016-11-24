#ifndef PT_APPLYALIGNMENT_H
#define PT_APPLYALIGNMENT_H

#include "processors/processor.h"

namespace Mechanics {
class Device;
}

namespace Processors {

void setGeometry(Storage::Event* event, const Mechanics::Device* device);

class ApplyAlignment : public Processor {
public:
  ApplyAlignment(const Mechanics::Device& device);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Device& m_device;
};

} // namespace Processors

#endif // PT_APPLYALIGNMENT_H
