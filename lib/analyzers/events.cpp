#include "events.h"

#include <cassert>
#include <cmath>
#include <cstdint>

#include <TDirectory.h>
#include <TH1D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::Events::Events(const Mechanics::Device* device,
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

std::string Analyzers::Events::name() const { return "Events"; }

void Analyzers::Events::analyze(const Storage::Event& event)
{
  m_triggerOffset->Fill(event.triggerOffset());
  m_triggerPhase->Fill(event.triggerPhase());
}

void Analyzers::Events::finalize() {}
