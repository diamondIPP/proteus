// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#include "eventloop.h"

#include <cassert>
#include <chrono>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "loop/analyzer.h"
#include "loop/processor.h"
#include "loop/reader.h"
#include "loop/sensorprocessor.h"
#include "loop/writer.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/progress.h"
#include "utils/statistics.h"

namespace proteus {
namespace {

// Timing measurements for the different parts of the event loop
struct Timing {
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using Time = Clock::time_point;

  Time startTime;
  std::vector<Duration> io;
  std::vector<Duration> processors;
  std::vector<Duration> analyzers;
  std::vector<std::string> namesIo;
  std::vector<std::string> namesProcessors;
  std::vector<std::string> namesAnalyzers;

  Timing(std::vector<std::string> namesIo_,
         std::vector<std::string> namesProcessors_,
         std::vector<std::string> namesAnalyzers_)
      : startTime(Clock::now())
      , io(namesIo_.size(), Duration::zero())
      , processors(namesProcessors_.size(), Duration::zero())
      , analyzers(namesAnalyzers_.size(), Duration::zero())
      , namesIo(std::move(namesIo_))
      , namesProcessors(std::move(namesProcessors_))
      , namesAnalyzers(std::move(namesAnalyzers_))
  {
  }

  void summarize(uint64_t numEvents) const
  {
    // compute total time spent
    auto sum_total = [](const std::vector<Duration>& durations) {
      return std::accumulate(durations.begin(), durations.end(),
                             Duration::zero());
    };
    // print timing
    // allow fractional tics when calculating time per event
    auto time_per_event_us = [=](const Duration& dt) {
      std::ostringstream s;
      s << (std::chrono::duration<float, std::micro>(dt) / numEvents).count()
        << " us/event";
      return s.str();
    };
    auto time_min_s = [](const Duration& dt) {
      std::ostringstream s;
      s << std::chrono::duration_cast<std::chrono::minutes>(dt).count()
        << " min "
        << std::chrono::duration_cast<std::chrono::seconds>(dt).count() % 60
        << " s";
      return s.str();
    };
    auto print_times = [&](const std::vector<Duration>& durations,
                           const std::vector<std::string>& names) {
      for (size_t i = 0; i < std::min(durations.size(), names.size()); ++i) {
        VERBOSE("    ", names[i], ": ", time_per_event_us(durations[i]));
      }
    };

    auto stopTime = Clock::now();
    auto total_io = sum_total(io);
    auto total_processors = sum_total(processors);
    auto total_analyzers = sum_total(analyzers);
    auto total = total_io + total_processors + total_analyzers;

    VERBOSE("time: ", time_per_event_us(total));
    VERBOSE("  io: ", time_per_event_us(total_io));
    print_times(io, namesIo);
    VERBOSE("  processors: ", time_per_event_us(total_processors));
    print_times(processors, namesProcessors);
    VERBOSE("  analyzers: ", time_per_event_us(total_analyzers));
    print_times(analyzers, namesAnalyzers);
    VERBOSE("time (clocked): ", time_min_s(total));
    VERBOSE("time (wall): ", time_min_s(stopTime - startTime));
  }
};

// RAII-based stop-watch that adds time to the given duration
struct StopWatch {
  Timing::Duration& clock;
  Timing::Time start;

  StopWatch(Timing::Duration& clock_)
      : clock(clock_), start(Timing::Clock::now())
  {
  }
  ~StopWatch() { clock += Timing::Clock::now() - start; }
};

// Summary statistics for basic event information.
struct Statistics {
  uint64_t events = 0;
  StatAccumulator<uint64_t> hits, clusters, tracks;

  void fill(uint64_t nHits, uint64_t nClusters, uint64_t nTracks)
  {
    events += 1;
    hits.fill(nHits);
    clusters.fill(nClusters);
    tracks.fill(nTracks);
  }
  void summarize() const
  {
    INFO("processed ", events, " events");
    VERBOSE("  hits/event: ", hits);
    VERBOSE("  clusters/event: ", clusters);
    VERBOSE("  tracks/event: ", tracks);
  }
};

} // namespace

EventLoop::EventLoop(std::shared_ptr<Reader> reader,
                     size_t sensors,
                     uint64_t start,
                     uint64_t events,
                     bool showProgress)
    : m_reader(std::move(reader))
    , m_start(start)
    , m_events(0)
    , m_sensors(sensors)
    , m_showProgress(showProgress)
{
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
      m_events = events;
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

EventLoop::~EventLoop() {}

void EventLoop::addSensorProcessor(
    size_t sensorId, std::shared_ptr<SensorProcessor> sensorProcessor)
{
  m_sensorProcessors[sensorId].emplace_back(std::move(sensorProcessor));
}

void EventLoop::addProcessor(std::shared_ptr<Processor> processor)
{
  m_processors.emplace_back(std::move(processor));
}

void EventLoop::addAnalyzer(std::shared_ptr<Analyzer> analyzer)
{
  m_analyzers.emplace_back(std::move(analyzer));
}

void EventLoop::addWriter(std::shared_ptr<Writer> writer)
{
  m_writers.emplace_back(std::move(writer));
}

void EventLoop::run()
{
  // create list of names for all configured algorithms
  std::vector<std::string> namesIo;
  namesIo.emplace_back(m_reader->name());
  DEBUG("configured readers:");
  DEBUG("  ", namesIo.back());
  std::vector<std::string> namesProcessors;
  for (const auto& sp : m_sensorProcessors) {
    DEBUG("configured processors for sensor ", sp.first);
    for (const auto& sensorProcessor : sp.second) {
      std::string name = "Sensor(id=";
      name += std::to_string(sp.first);
      name += "):";
      name += sensorProcessor->name();
      namesProcessors.emplace_back(std::move(name));
      DEBUG("  ", namesProcessors.back());
    }
  }
  DEBUG("configured processors:");
  for (const auto& processor : m_processors) {
    namesProcessors.emplace_back(processor->name());
    DEBUG("  ", namesProcessors.back());
  }
  std::vector<std::string> namesAnalyzers;
  DEBUG("configured analyzers:");
  for (const auto& analyzer : m_analyzers) {
    namesAnalyzers.emplace_back(analyzer->name());
    DEBUG("  ", namesAnalyzers.back());
  }
  DEBUG("configured writers:");
  for (const auto& writer : m_writers) {
    namesIo.emplace_back(writer->name());
    DEBUG("  ", namesIo.back());
  }

  // setup timing, statistics, and progress
  Timing timing(namesIo, namesProcessors, namesAnalyzers);
  Statistics stats;
  Progress progress(m_showProgress ? m_events : 0);
  progress.update(0);

  // start event loop proper
  Event event(m_sensors);
  {
    StopWatch sw(timing.io[0]);
    m_reader->skip(m_start);
  }
  uint64_t processed = 0;
  for (; processed < m_events; ++processed) {
    {
      StopWatch sw(timing.io[0]);
      if (!m_reader->read(event)) {
        break;
      }
    }
    size_t iprocessor = 0;
    for (auto& sp : m_sensorProcessors) {
      // select the corresponding sensor event
      auto& sensorEvent = event.getSensorEvent(sp.first);
      // and execute all configured per-sensor processors for it
      for (auto& sensorProcessor : sp.second) {
        StopWatch sw(timing.processors[iprocessor++]);
        sensorProcessor->execute(sensorEvent);
      }
    }
    for (auto& processor : m_processors) {
      StopWatch sw(timing.processors[iprocessor++]);
      processor->execute(event);
    }
    size_t ianalyzer = 0;
    for (auto& analyzer : m_analyzers) {
      StopWatch sw(timing.analyzers[ianalyzer++]);
      analyzer->execute(event);
    }
    // first io entry is always the reader
    size_t iio = 1;
    for (auto& writer : m_writers) {
      StopWatch sw(timing.io[iio++]);
      writer->append(event);
    }
    stats.fill(event.getNumHits(), event.getNumClusters(), event.numTracks());
    progress.update(processed + 1);
  }
  progress.clear();
  size_t ianalyzer = 0;
  for (const auto& analyzer : m_analyzers) {
    StopWatch sw(timing.analyzers[ianalyzer++]);
    analyzer->finalize();
  }
  timing.summarize(processed + 1);
  stats.summarize();
}

} // namespace proteus
