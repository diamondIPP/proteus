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
  Duration reader = Duration::zero();
  std::vector<Duration> processors;
  std::vector<Duration> analyzers;
  std::vector<Duration> writers;

  Timing(size_t numProcessors, size_t numAnalyzers, size_t numWriters)
      : processors(numProcessors, Duration::zero())
      , analyzers(numAnalyzers, Duration::zero())
      , writers(numWriters, Duration::zero())
  {
  }

  static Time now() { return Clock::now(); }
  void start() { start_ = Clock::now(); }
  void stop() { stop_ = Clock::now(); }
  template <typename Processors, typename Analyzers, typename Writers>
  void summarize(uint64_t numEvents,
                 const Processors& ps,
                 const Analyzers& as,
                 const Writers& ws) const
  {
    auto pros =
        std::accumulate(processors.begin(), processors.end(), Duration::zero());
    auto anas =
        std::accumulate(analyzers.begin(), analyzers.end(), Duration::zero());
    auto wrts =
        std::accumulate(writers.begin(), writers.end(), Duration::zero());
    auto total = reader + pros + anas + wrts;

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
    INFO("  reader: ", time_us_per_event(reader));
    INFO("  processors: ", time_us_per_event(pros));
    size_t ip = 0;
    for (const auto& p : ps) {
      DEBUG("    ", p->name(), ": ", time_us_per_event(processors[ip++]));
    }
    INFO("  analyzers: ", time_us_per_event(anas));
    size_t ia = 0;
    for (const auto& a : as) {
      DEBUG("    ", a->name(), ": ", time_us_per_event(analyzers[ia++]));
    }
    INFO("  writers: ", time_us_per_event(wrts));
    size_t iw = 0;
    for (const auto& w : ws) {
      DEBUG("    ", w->name(), ": ", time_us_per_event(writers[iw++]));
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
} // namespace

Utils::EventLoop::EventLoop(Io::EventReader* reader,
                            uint64_t start,
                            uint64_t events)
    : m_reader(reader), m_start(start), m_events(0), m_showProgress(false)
{
  assert(reader && "EventReader must be non-NULL");

  uint64_t available = m_reader->numEvents();

  DEBUG("requested start: ", start);
  DEBUG("requested events: ", events);
  DEBUG("available events: ", available);

  if (available <= start) {
    FAIL("start event ", start, " exceeds available ", available, " events");
  }
  // case 1: users explicitly requested a specific number of events
  if (events != UINT64_MAX) {
    // there are less events available than requested
    if (available < (start + events)) {
      m_events = available - start;
      INFO("restrict to ", available, " events available");
    // there are enought events available
    } else {
      m_events = events - start;
    }
  // case 2: users wants to process all events available
  } else {
    // number of events is known
    if (available != UINT64_MAX) {
      m_events = available - start;
    // number of events is unkown
    } else {
      m_events = UINT64_MAX;
    }
  }
}

Utils::EventLoop::~EventLoop() {}

void Utils::EventLoop::enableProgressBar() { m_showProgress = true; }

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

void Utils::EventLoop::addWriter(std::shared_ptr<Io::EventWriter> writer)
{
  m_writers.emplace_back(std::move(writer));
}

void Utils::EventLoop::run()
{
  Timing timing(m_processors.size(), m_analyzers.size(), m_writers.size());
  Statistics stats;
  Storage::Event event;

  DEBUG("configured readers:");
  DEBUG("  ", m_reader->name());
  DEBUG("configured processors:");
  for (const auto& processor : m_processors)
    DEBUG("  ", processor->name());
  DEBUG("configured analyzers:");
  for (const auto& analyzer : m_analyzers)
    DEBUG("  ", analyzer->name());
  DEBUG("configured writers:");
  for (const auto& writer : m_writers)
    DEBUG("  ", writer->name());

  Utils::ProgressBar progress;
  if (m_showProgress)
    progress.update<uint64_t>(0, m_events);
  timing.start();
  {
    auto start = Timing::now();
    m_reader->skip(m_start);
    timing.reader += Timing::now() - start;
  }
  for (uint64_t nevents = 1; nevents <= m_events; ++nevents) {
    {
      auto start = Timing::now();
      bool areEventsLeft = m_reader->readNext(event);
      timing.reader += Timing::now() - start;
      if (!areEventsLeft)
        break;
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
    for (size_t i = 0; i < m_writers.size(); ++i) {
      auto start = Timing::now();
      m_writers[i]->append(event);
      timing.writers[i] += Timing::now() - start;
    }
    stats.fill(event.getNumHits(), event.getNumClusters(), event.numTracks());
    if (m_showProgress)
      progress.update(nevents, m_events);
  }
  if (m_showProgress)
    progress.clear();
  for (const auto& analyzer : m_analyzers)
    analyzer->finalize();
  timing.stop();
  timing.summarize(m_events, m_processors, m_analyzers, m_writers);
  stats.summarize();
}
