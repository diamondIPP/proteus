#ifndef PT_EVENTS_H
#define PT_EVENTS_H

#include <vector>

#include "analyzer.h"

class TDirectory;
class TH1D;

namespace Mechanics {
class Device;
}

namespace Analyzers {

/** Overall event information, e.g. timing and hit and cluster rates. */
class Events : public Analyzer {
public:
  Events(TDirectory* dir,
         const Mechanics::Device& device,
         /* Histogram options */
         const int binsTimestamps = 1024);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  TH1D* m_triggerOffset;
  TH1D* m_triggerPhase;
};

} // namespace Analyzers

#endif // PT_EVENTINFO_H
