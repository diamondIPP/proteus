/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#include "eventloop.h"

#include <cassert>
#include <chrono>
#include <numeric>

#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/logger.h"
#include "utils/progressbar.h"
#include "utils/statistics.h"

PT_SETUP_LOCAL_LOGGER(EventLoop)

namespace {
// Summary statistics for basic event information.
struct Statistics {
  uint64_t events = 0;
  uint64_t eventsWithTracks = 0;
  Utils::StatAccumulator<uint64_t> hits, clusters, tracks;

  void fill(uint64_t nHits, uint64_t nClusters, uint64_t nTracks)
  {
    events += 1;
    eventsWithTracks += (0 < nTracks) ? 1 : 0;
    hits.fill(nHits);
    clusters.fill(nClusters);
    tracks.fill(nTracks);
  }
  void summarize()
  {
    INFO("events (with tracks/total): ", eventsWithTracks, "/", events);
    INFO("hits/event: ", hits);
    INFO("clusters/event: ", clusters);
    INFO("tracks/event: ", tracks);
  }
};
}

Utils::EventLoop::EventLoop(Storage::StorageIO* input,
                            uint64_t start,
                            uint64_t events)
    : m_input(input), m_output(NULL), m_startEvent(start), m_showProgress(false)
{
  assert(input && "input storage is NULL");

  uint64_t eventsOnFile = m_input->getNumEvents();

  DEBUG("requested start: ", start);
  DEBUG("requested events: ", events);
  DEBUG("available events: ", eventsOnFile);

  if (eventsOnFile <= start)
    FAIL("start event exceeeds available events: start=", start, ", events=",
         eventsOnFile);
  if (events == UINT64_MAX) {
    m_numEvents = eventsOnFile - m_startEvent;
  } else {
    if (eventsOnFile < (start + events)) {
      m_numEvents = eventsOnFile - m_startEvent;
      INFO("restrict to available number of events ", m_numEvents);
    } else {
      m_numEvents = events;
    }
  }
}

Utils::EventLoop::~EventLoop() {}

void Utils::EventLoop::enableProgressBar() { m_showProgress = true; }

void Utils::EventLoop::setOutput(Storage::StorageIO* output)
{
  assert(output && "ouput storage is NULL");

  m_output = output;
};

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

void Utils::EventLoop::run()
{
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using Time = Clock::time_point;

  Storage::Event event(m_input->getNumPlanes());
  Statistics stats;
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
  if (m_showProgress)
    progress.update(0);
  Time startWall = Clock::now();
  for (uint64_t ievent = 0; ievent < m_numEvents; ++ievent) {
    {
      Time start = Clock::now();
      m_input->readEvent(m_startEvent + ievent, &event);
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
    if (m_showProgress)
      progress.update((float)(ievent + 1) / numEvents());
  }
  if (m_showProgress)
    progress.clear();

  for (auto a = m_analyzers.begin(); a != m_analyzers.end(); ++a) {
    (*a)->finalize();
  }
  Duration durationWall = Clock::now() - startWall;
  Duration durationProc = std::accumulate(
      durationProcs.begin(), durationProcs.end(), Duration::zero());
  Duration durationAna = std::accumulate(durationAnas.begin(),
                                         durationAnas.end(), Duration::zero());
  Duration durationTotal =
      durationInput + durationProc + durationAna + durationOutput;

  // print timing
  // allow fractional tics when calculating time per event
  const char* unit = " us/event";
  auto time_per_event = [&](const Duration& dt) -> float {
    return (std::chrono::duration<float, std::micro>(dt) / m_numEvents).count();
  };
  INFO("time: ", time_per_event(durationTotal), unit);
  INFO("  input: ", time_per_event(durationInput), unit);
  if (m_output)
    INFO("  output: ", time_per_event(durationOutput), unit);
  INFO("  processors: ", time_per_event(durationProc), unit);
  for (size_t i = 0; i < m_processors.size(); ++i) {
    DEBUG("    ", m_processors[i]->name(), ": ",
          time_per_event(durationProcs[i]), unit);
  }
  INFO("  analyzers: ", time_per_event(durationAna), unit);
  for (size_t i = 0; i < m_analyzers.size(); ++i) {
    DEBUG("    ", m_analyzers[i]->name(), ": ", time_per_event(durationAnas[i]),
          unit);
  }
  INFO("time (clocked): ",
       std::chrono::duration_cast<std::chrono::minutes>(durationTotal).count(),
       " min ",
       std::chrono::duration_cast<std::chrono::seconds>(durationTotal).count(),
       " s");
  INFO("time (wall): ",
       std::chrono::duration_cast<std::chrono::minutes>(durationWall).count(),
       " min ",
       std::chrono::duration_cast<std::chrono::seconds>(durationWall).count(),
       " s");
  stats.summarize();
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readStartEvent()
{
  return std::unique_ptr<Storage::Event>(m_input->readEvent(m_startEvent));
}

std::unique_ptr<Storage::Event> Utils::EventLoop::readEndEvent()
{
  return std::unique_ptr<Storage::Event>(
      m_input->readEvent(m_startEvent + m_numEvents - 1));
}
