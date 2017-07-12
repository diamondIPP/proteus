#include "eventinfo.h"

#include <cassert>
#include <cmath>
#include <cstdint>

#include <TDirectory.h>
#include <TH1D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(EventInfo)

Analyzers::EventInfo::EventInfo(const Mechanics::Device* device,
                                TDirectory* dir,
                                const int binsTimestamps)
{
  using namespace Utils;

  assert(device && "Analyzer: can't initialize with null device");

  int triggerMax = device->readoutWindow();

  TDirectory* sub = makeDir(dir, "events");
  m_triggerOffset =
      makeH1(sub, "trigger_offset", HistAxis{0, triggerMax, "Trigger offset"});
  m_triggerPhase =
      makeH1(sub, "trigger_phase", HistAxis{0, triggerMax, "Trigger phase"});
}

std::string Analyzers::EventInfo::name() const { return "EventInfo"; }

void Analyzers::EventInfo::analyze(const Storage::Event& event)
{
  m_triggerOffset->Fill(event.triggerOffset());
  m_triggerPhase->Fill(event.triggerPhase());
}

void Analyzers::EventInfo::finalize() {}
