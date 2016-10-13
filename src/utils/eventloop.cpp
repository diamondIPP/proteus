/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \created 2016-08
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

using Utils::logger;

Utils::EventLoop::EventLoop(Storage::StorageIO* storage,
                            uint64_t startEvent,
                            uint64_t numEvents)
    : m_storage(storage)
    , m_startEvent(startEvent)
{
  assert(storage && "storage is NULL");

  uint64_t eventsOnFile = m_storage->getNumEvents();
  if (numEvents == 0) {
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

  Storage::Event event(m_storage->getNumPlanes());
  Utils::EventStatistics stats;
  Duration durationWall = Duration::zero();
  Duration durationTotal = Duration::zero();
  Duration durationIo = Duration::zero();
  std::vector<Duration> durationProcs(m_processors.size(), Duration::zero());
  std::vector<Duration> durationAnas(m_analyzers.size(), Duration::zero());

  if (!m_processors.empty()) {
    DEBUG("configured processors:\n");
    for (auto p = m_processors.begin(); p != m_processors.end(); ++p)
      DEBUG("  ", (*p)->name(), '\n');
  }
  if (!m_analyzers.empty()) {
    DEBUG("configured analyzers:\n");
    for (auto a = m_analyzers.begin(); a != m_analyzers.end(); ++a)
      DEBUG("  ", (*a)->name(), '\n');
  }

  Utils::ProgressBar progress;
  Time startWall = Clock::now();
  for (uint64_t ievent = m_startEvent; ievent <= m_endEvent; ievent++) {

    Time startIo = Clock::now();
    m_storage->readEvent(ievent, &event);
    durationIo += Clock::now() - startIo;

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

    stats.fill(event.getNumHits(), event.getNumClusters(), event.numTracks());
    progress.update((float)(ievent - m_startEvent + 1) / numEvents());
  }
  progress.clear();

  for (auto a = m_analyzers.begin(); a != m_analyzers.end(); ++a) {
    (*a)->finalize();
  }
  durationWall = Clock::now() - startWall;
  durationTotal = durationIo;
  durationTotal += std::accumulate(
      durationProcs.begin(), durationProcs.end(), Duration::zero());
  durationTotal += std::accumulate(
      durationAnas.begin(), durationAnas.end(), Duration::zero());

  // print common event statistics
  INFO("\nEvent Statistics:\n", stats.str("  "), '\n');

  // print timing
  // allow fractional tics when calculating time per event
  const char* unit = " us/event";
  auto time_per_event = [&](const Duration& dt) -> float {
    float n = m_endEvent - m_startEvent + 1;
    return (std::chrono::duration<float, std::micro>(dt) / n).count();
  };
  INFO("Timing:\n");
  INFO("  i/o: ", time_per_event(durationIo), unit, '\n');
  // INFO("  processors: ", time_per_event(durProcessors), unit);
  INFO("  processors:\n");
  for (size_t i = 0; i < m_processors.size(); ++i) {
    INFO("    ", m_processors[i]->name(), ": ");
    INFO(time_per_event(durationProcs[i]), unit, '\n');
  }
  INFO("  analyzers:\n");
  for (size_t i = 0; i < m_analyzers.size(); ++i) {
    INFO("    ", m_analyzers[i]->name(), ": ");
    INFO(time_per_event(durationAnas[i]), unit, '\n');
  }
  INFO("  combined: ", time_per_event(durationTotal), unit, '\n');
  INFO("  total (clocked): ",
       std::chrono::duration_cast<std::chrono::minutes>(durationTotal).count(),
       " min ",
       std::chrono::duration_cast<std::chrono::seconds>(durationTotal).count(),
       " s\n");
  INFO("  total (wall): ",
       std::chrono::duration_cast<std::chrono::minutes>(durationWall).count(),
       " min ",
       std::chrono::duration_cast<std::chrono::seconds>(durationWall).count(),
       " s\n");
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readStartEvent()
{
  return std::unique_ptr<Storage::Event>(m_storage->readEvent(m_startEvent));
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readEndEvent()
{
  return std::unique_ptr<Storage::Event>(m_storage->readEvent(m_endEvent));
}
