/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#include "eventloop.h"

#include <cassert>
#include <chrono>
#include <numeric>
#include <sstream>

#include "storage/event.h"
#include "storage/storageio.h"
#include "utils/logger.h"
#include "utils/progressbar.h"
#include "utils/statistics.h"

PT_SETUP_LOCAL_LOGGER(EventLoop)

namespace {

// Timing measurements for the different parts of the event loop
struct Timing {
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using Time = Clock::time_point;

  Time start_, stop_;
  Duration input = Duration::zero();
  Duration output = Duration::zero();
  std::vector<Duration> processors;
  std::vector<Duration> analyzers;

  Timing(size_t numProcessors, size_t numAnalyzers)
      : processors(numProcessors, Duration::zero())
      , analyzers(numAnalyzers, Duration::zero())
  {
  }

  static Time now() { return Clock::now(); }
  void start() { start_ = Clock::now(); }
  void stop() { stop_ = Clock::now(); }
  template <typename Processors, typename Analyzers>
  void
  summarize(uint64_t numEvents, const Processors& ps, const Analyzers& as) const
  {
    auto proc =
        std::accumulate(processors.begin(), processors.end(), Duration::zero());
    auto anas =
        std::accumulate(analyzers.begin(), analyzers.end(), Duration::zero());
    auto total = input + output + proc + anas;

    // print timing
    // allow fractional tics when calculating time per event
    auto time_us_per_event = [&](const Duration& dt) {
      std::ostringstream s;
      s << (std::chrono::duration<float, std::micro>(dt) / numEvents).count()
        << " us/event";
      return s.str();
    };
    auto time_min_s = [](const Duration& dt) {
      std::ostringstream s;
      s << std::chrono::duration_cast<std::chrono::minutes>(dt).count()
        << " min "
        << std::chrono::duration_cast<std::chrono::seconds>(dt).count() << " s";
      return s.str();
    };

    INFO("time: ", time_us_per_event(total));
    INFO("  input: ", time_us_per_event(input));
    INFO("  output: ", time_us_per_event(output));
    INFO("  processors: ", time_us_per_event(proc));
    size_t ip = 0;
    for (const auto& p : ps) {
      DEBUG("    ", p->name(), ": ", time_us_per_event(processors[ip++]));
    }
    INFO("  analyzers: ", time_us_per_event(anas));
    size_t ia = 0;
    for (const auto& a : as) {
      DEBUG("    ", a->name(), ": ", time_us_per_event(analyzers[ia++]));
    }
    INFO("time (clocked): ", time_min_s(total));
    INFO("time (wall): ", time_min_s(stop_ - start_));
  }
};

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
  void summarize() const
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
  Storage::Event event(m_input->getNumPlanes());
  Statistics stats;
  Timing timing(m_processors.size(), m_analyzers.size());

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
  timing.start();
  for (uint64_t ievent = 0; ievent < m_numEvents; ++ievent) {
    {
      auto start = Timing::now();
      m_input->readEvent(m_startEvent + ievent, &event);
      timing.input += Timing::now() - start;
    }
    for (size_t i = 0; i < m_processors.size(); ++i) {
      auto start = Timing::now();
      m_processors[i]->process(event);
      timing.processors[i] += Timing::now() - start;
    }
    for (size_t i = 0; i < m_analyzers.size(); ++i) {
      auto start = Timing::now();
      m_analyzers[i]->analyze(event);
      timing.analyzers[i] += Timing::now() - start;
    }
    if (m_output) {
      auto start = Timing::now();
      m_output->writeEvent(&event);
      timing.output += Timing::now() - start;
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
  timing.stop();
  timing.summarize(numEvents(), m_processors, m_analyzers);
  stats.summarize();
}
