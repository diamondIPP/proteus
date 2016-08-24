/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \created 2016-08
 */

#include <cassert>
#include <chrono>

#include "eventloop.h"
#include "storage/event.h"
#include "storage/storageio.h"

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
    std::unique_ptr<Processors::Processor> processor)
{
  m_processors.emplace_back(std::move(processor));
}

void Utils::EventLoop::addAnalyzer(
    std::unique_ptr<Analyzers::Analyzer> analyzer)
{
  m_analyzers.emplace_back(std::move(analyzer));
}

void Utils::EventLoop::run()
{
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using Time = Clock::time_point;

  Duration durIo = Duration::zero();
  Duration durProcessors = Duration::zero();
  Duration durAnalyzers = Duration::zero();
  Duration durTotal = Duration::zero();
  std::unique_ptr<Storage::Event> event;

  for (uint64_t ievent = m_startEvent; ievent <= m_endEvent; ievent++) {

    Time startIo = Clock::now();
    event.reset(m_storage->readEvent(ievent));

    Time startProcessors = Clock::now();
    for (auto p = m_processors.begin(); p != m_processors.end(); ++p)
      (*p)->processEvent(ievent, *event.get());

    Time startAnalyzers = Clock::now();
    for (auto a = m_analyzers.begin(); a != m_analyzers.end(); ++a)
      (*a)->analyze(ievent, *event.get());

    Time end = Clock::now();
    durIo += startProcessors - startIo;
    durProcessors += startAnalyzers - startProcessors;
    durAnalyzers += end - startAnalyzers;
    durTotal += end - startIo;

    m_stat.fill(
        event->getNumHits(), event->getNumClusters(), event->getNumTracks());
    m_progress.update((float)(ievent - m_startEvent + 1) / numEvents());
  }

  for (auto p = m_processors.begin(); p != m_processors.end(); ++p)
    (*p)->finalize();
  for (auto a = m_analyzers.begin(); a != m_analyzers.end(); ++a)
    (*a)->finalize();

  using std::cout;

  // print common event statistics
  cout << "\nEvent Statistics:\n" << m_stat.str("  ") << '\n';

  // print timing
  // allow fractional tics when calculating time per event
  const char unit[] = " us/event\n";
  auto time_per_event = [&](const Duration& dt) -> float {
    float n = m_endEvent - m_startEvent + 1;
    return (std::chrono::duration<float, std::micro>(dt) / n).count();
  };
  cout << "Timing:\n";
  cout << "  io: " << time_per_event(durIo) << unit;
  cout << "  processors: " << time_per_event(durProcessors) << unit;
  cout << "  analyzers: " << time_per_event(durAnalyzers) << unit;
  cout << "  total: " << time_per_event(durTotal) << unit;
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readStartEvent()
{
  return std::unique_ptr<Storage::Event>(m_storage->readEvent(m_startEvent));
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readEndEvent()
{
  return std::unique_ptr<Storage::Event>(m_storage->readEvent(m_endEvent));
}
