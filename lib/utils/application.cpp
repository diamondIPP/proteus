/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-12-12
 */

#include "application.h"

#include <cstdlib>

#include "analyzers/eventprinter.h"
#include "io/open.h"
#include "mechanics/device.h"
#include "utils/arguments.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Application)

Utils::Application::Application(const std::string& name,
                                const std::string& description,
                                const toml::Table& defaults)
    : m_name(name)
    , m_desc(description)
    , m_cfg(defaults)
    , m_printEvents(false)
    , m_showProgress(false)
{
}

void Utils::Application::initialize(int argc, char const* argv[])
{
  Utils::Arguments args(m_desc);
  args.addOption('d', "device", "device configuration file", "device.toml");
  args.addOption('g', "geometry", "use a different geometry file");
  args.addOptionMulti('m', "mask", "load additional pixel mask file");
  args.addOption('c', "config", "analysis configuration file", "analysis.toml");
  args.addOption('u', "subsection", "use the given configuration sub-section");
  args.addOption('s', "skip_events", "skip the first n events", 0);
  args.addOption('n', "num_events", "number of events to process", UINT64_MAX);
  args.addFlag('q', "quiet", "print only errors");
  args.addFlag('v', "verbose", "print more information");
  args.addFlag('\0', "print-events", "print full event information");
  args.addFlag('\0', "no-progress", "do not show a progress bar");
  args.addRequired("input", "path to the input file");
  args.addRequired("output_prefix", "output path prefix");

  // parse should print help automatically
  if (args.parse(argc, argv))
    std::exit(EXIT_FAILURE);

  // logging level
  if (args.has("quiet")) {
    Utils::Logger::setGlobalLevel(Utils::Logger::Level::Error);
  } else if (args.has("verbose")) {
    Utils::Logger::setGlobalLevel(Utils::Logger::Level::Verbose);
  } else {
    Utils::Logger::setGlobalLevel(Utils::Logger::Level::Info);
  }
  // event printing
  if (args.has("print-events")) {
    m_printEvents = true;
  }
  // hide progress-bar
  if (!args.has("no-progress")) {
    m_showProgress = true;
  }

  // select configuration (sub-)section
  std::string section = m_name;
  if (args.has("subsection")) {
    section += '.';
    section += args.get("subsection");
  }

  // load device w/ optional geometry override
  auto pathDev = args.get("device");
  auto pathGeo = (args.has("geometry") ? args.get("geometry") : std::string());
  auto dev = Mechanics::Device::fromFile(pathDev, pathGeo);
  m_dev.reset(new Mechanics::Device(std::move(dev)));

  // load additional pixel masks
  if (args.has("mask")) {
    for (const auto& maskPath : args.get<std::vector<std::string>>("mask")) {
      m_dev->applyPixelMasks(Mechanics::PixelMasks::fromFile(maskPath));
    }
  }

  // read analysis configuration w/ automatic handling of defaults
  const toml::Value cfgAll = Utils::Config::readConfig(args.get("config"));
  const toml::Value* cfg = cfgAll.find(section);
  if (!cfg)
    FAIL("configuration section '", section, "' is missing");
  m_cfg = Utils::Config::withDefaults(*cfg, m_cfg);
  INFO("read configuration '", section, "' from '", args.get("config"), "'");

  // setup paths and i/o settings
  m_inputPath = args.get("input");
  m_outputPrefix = args.get("output_prefix");
  m_skipEvents = args.get<uint64_t>("skip_events");
  m_numEvents = args.get<uint64_t>("num_events");
}

std::string Utils::Application::outputPath(const std::string& name) const
{
  return m_outputPrefix + '-' + name;
}

Loop::EventLoop Utils::Application::makeEventLoop() const
{
  // NOTE open the file just when the event loop is created to ensure that the
  //      input reader always starts at the beginning of the file.
  Loop::EventLoop loop(Io::openRead(m_inputPath, toml::Value()),
                       m_dev->numSensors(), m_skipEvents, m_numEvents,
                       m_showProgress);
  // full-event output in debug mode
  if (m_printEvents) {
    loop.addAnalyzer(std::make_shared<Analyzers::EventPrinter>());
  }
  return loop;
}
