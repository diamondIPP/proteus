/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#include "eventloop.h"

#include <cassert>
#include <chrono>
#include <numeric>

#include "analyzers/singleanalyzer.h"
#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/logger.h"
#include "utils/progressbar.h"
#include "utils/statistics.h"

PT_SETUP_LOCAL_LOGGER(EventLoop)

Utils::EventLoop::EventLoop(Storage::StorageIO* input,
                            uint64_t startEvent,
                            uint64_t numEvents)
    : EventLoop(input, NULL, startEvent, numEvents)
{
}

Utils::EventLoop::EventLoop(Storage::StorageIO* input,
                            Storage::StorageIO* output,
                            uint64_t startEvent,
                            uint64_t numEvents)
    : m_input(input), m_output(output), m_startEvent(startEvent)
{
  assert(input && "input storage is NULL");
  // output can be NULL

  uint64_t eventsOnFile = m_input->getNumEvents();
  if (numEvents == static_cast<uint64_t>(-1)) {
    m_endEvent = eventsOnFile - 1;
  } else {
    m_endEvent = m_startEvent + numEvents - 1;
  }

  if (eventsOnFile < m_startEvent)
    throw std::runtime_error("start event exceeds available events in storage");
  if (eventsOnFile < m_endEvent)
    throw std::runtime_error("end event exceeds available events in storage");
}

Utils::EventLoop::~EventLoop() {}

void Utils::EventLoop::addProcessor(
    std::shared_ptr<Processors::Processor> processor)
{
  m_processors.emplace_back(std::move(processor));
}

void Utils::EventLoop::addAnalyzer(
    std::shared_ptr<Analyzers::Analyzer> analyzer)
{
  m_analyzers.emplace_back(std::move(analyzer));
}

/** Wrapper for SingleAnalyzer to conform to new interface. */
class SingleAnalyzerShim : public Analyzers::Analyzer {
public:
  SingleAnalyzerShim(std::shared_ptr<Analyzers::SingleAnalyzer>&& analyzer)
      : m_ana(std::move(analyzer))
  {
  }
  std::string name() const { return m_ana->name(); };
  void analyze(const Storage::Event& event) { m_ana->processEvent(&event); };
  void finalize() { m_ana->postProcessing(); };

private:
  std::shared_ptr<Analyzers::SingleAnalyzer> m_ana;
};

void Utils::EventLoop::addAnalyzer(
    std::shared_ptr<Analyzers::SingleAnalyzer> analyzer)
{
  m_analyzers.emplace_back(
      std::make_shared<SingleAnalyzerShim>(std::move(analyzer)));
}

void Utils::EventLoop::run()
{
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using Time = Clock::time_point;

  Storage::Event event(m_input->getNumPlanes());
  Utils::EventStatistics stats;
  Duration durationWall = Duration::zero();
  Duration durationTotal = Duration::zero();
  Duration durationInput = Duration::zero();
  Duration durationOutput = Duration::zero();
  std::vector<Duration> durationProcs(m_processors.size(), Duration::zero());
  std::vector<Duration> durationAnas(m_analyzers.size(), Duration::zero());

  if (!m_processors.empty()) {
    DEBUG("configured processors:");
    for (auto p = m_processors.begin(); p != m_processors.end(); ++p)
      DEBUG("  ", (*p)->name());
  }
  if (!m_analyzers.empty()) {
    DEBUG("configured analyzers:");
    for (auto a = m_analyzers.begin(); a != m_analyzers.end(); ++a)
      DEBUG("  ", (*a)->name());
  }

  Utils::ProgressBar progress;
  Time startWall = Clock::now();
  for (uint64_t ievent = m_startEvent; ievent <= m_endEvent; ievent++) {

    {
      Time start = Clock::now();
      m_input->readEvent(ievent, &event);
      durationInput += Clock::now() - start;
    }
    for (size_t i = 0; i < m_processors.size(); ++i) {
      Time start = Clock::now();
      m_processors[i]->process(event);
      durationProcs[i] += Clock::now() - start;
    }
    for (size_t i = 0; i < m_analyzers.size(); ++i) {
      Time start = Clock::now();
      m_analyzers[i]->analyze(event);
      durationAnas[i] += Clock::now() - start;
    }
    if (m_output) {
      Time start = Clock::now();
      m_output->writeEvent(&event);
      durationOutput += Clock::now() - start;
    }

    stats.fill(event.getNumHits(), event.getNumClusters(), event.numTracks());
    progress.update((float)(ievent - m_startEvent + 1) / numEvents());
  }
  progress.clear();

  for (auto a = m_analyzers.begin(); a != m_analyzers.end(); ++a) {
    (*a)->finalize();
  }
  durationWall = Clock::now() - startWall;
  durationTotal = durationInput + durationOutput;
  durationTotal += std::accumulate(durationProcs.begin(), durationProcs.end(),
                                   Duration::zero());
  durationTotal += std::accumulate(durationAnas.begin(), durationAnas.end(),
                                   Duration::zero());

  // print common event statistics
  INFO("\nEvent Statistics:\n", stats.str("  "));

  // print timing
  // allow fractional tics when calculating time per event
  const char* unit = " us/event";
  auto time_per_event = [&](const Duration& dt) -> float {
    float n = m_endEvent - m_startEvent + 1;
    return (std::chrono::duration<float, std::micro>(dt) / n).count();
  };
  INFO("Timing:");
  INFO("  input: ", time_per_event(durationInput), unit);
  if (m_output)
    INFO("  output: ", time_per_event(durationOutput), unit);
  INFO("  processors:");
  for (size_t i = 0; i < m_processors.size(); ++i) {
    INFO("    ", m_processors[i]->name(), ": ",
         time_per_event(durationProcs[i]), unit);
  }
  INFO("  analyzers:");
  for (size_t i = 0; i < m_analyzers.size(); ++i) {
    INFO("    ", m_analyzers[i]->name(), ": ", time_per_event(durationAnas[i]),
         unit);
  }
  INFO("  combined: ", time_per_event(durationTotal), unit);
  INFO("  total (clocked): ",
       std::chrono::duration_cast<std::chrono::minutes>(durationTotal).count(),
       " min ",
       std::chrono::duration_cast<std::chrono::seconds>(durationTotal).count(),
       " s");
  INFO("  total (wall): ",
       std::chrono::duration_cast<std::chrono::minutes>(durationWall).count(),
       " min ",
       std::chrono::duration_cast<std::chrono::seconds>(durationWall).count(),
       " s");
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readStartEvent()
{
  return std::unique_ptr<Storage::Event>(m_input->readEvent(m_startEvent));
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readEndEvent()
{
  return std::unique_ptr<Storage::Event>(m_input->readEvent(m_endEvent));
}
