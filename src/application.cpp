/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-12-12
 */

#include "application.h"

#include <cstdlib>

#include "mechanics/device.h"
#include "storage/storageio.h"
#include "utils/arguments.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Application)

Application::Application(const std::string& name,
                         const std::string& description,
                         const toml::Table& defaults)
    : m_name(name), m_desc(description), m_cfg(defaults), m_showProgress(false)
{
}

void Application::initialize(int argc, char const* argv[])
{
  Utils::Arguments args(m_desc);
  args.addOptional('d', "device", "device configuration file", "device.toml");
  args.addOptional('g', "geometry", "use a different geometry file");
  args.addMulti('m', "mask", "load additional pixel mask file");
  args.addOptional('c', "config", "analysis configuration file",
                   "analysis.toml");
  args.addOptional('u', "subsection",
                   "use the given configuration sub-section");
  args.addOptional('s', "skip_events", "skip the first n events", 0);
  args.addOptional('n', "num_events", "number of events to process",
                   UINT64_MAX);
  args.addFlag('q', "quiet", "print only errors");
  args.addFlag('\0', "debug", "print more information");
  args.addFlag('\0', "no-progress", "do not show a progress bar");
  args.addRequired("input", "path to the input file");
  args.addRequired("output_prefix", "output path prefix");

  // should print help automatically
  if (args.parse(argc, argv))
    std::exit(EXIT_FAILURE);

  // logging level
  if (args.has("quiet")) {
    Utils::Logger::setGlobalLevel(Utils::Logger::Level::Error);
  } else if (args.has("debug")) {
    Utils::Logger::setGlobalLevel(Utils::Logger::Level::Debug);
  } else {
    Utils::Logger::setGlobalLevel(Utils::Logger::Level::Info);
  }

  if (!args.has("no-progress"))
    m_showProgress = true;

  // define configuration section
  std::string section = m_name;
  if (args.has("subsection")) {
    section += '.';
    section += args.get("subsection");
  }
  // read configuration w/ automatic handling of defaults
  const toml::Value cfgAll = Utils::Config::readConfig(args.get("config"));
  const toml::Value* cfg = cfgAll.find(section);
  if (!cfg)
    FAIL("configuration section '", section, "' is missing");
  m_cfg = Utils::Config::withDefaults(*cfg, m_cfg);
  INFO("read configuration '", section, "' from '", args.get("config"), "'");

  // read device w/ optional geometry override and additional pixel masks
  m_dev.reset(
      new Mechanics::Device(Mechanics::Device::fromFile(args.get("device"))));
  if (args.has("geometry"))
    m_dev->setGeometry(Mechanics::Geometry::fromFile(args.get("geometry")));
  if (args.has("mask")) {
    auto maskPaths = args.get<std::vector<std::string>>("mask");
    for (const auto& path : maskPaths) {
      m_dev->applyPixelMasks(Mechanics::PixelMasks::fromFile(path));
    }
  }

  // setup input and i/o settings
  m_input.reset(new Storage::StorageIO(args.get("input"), Storage::INPUT,
                                       m_dev->numSensors()));
  m_outputPrefix = args.get("output_prefix");
  m_skipEvents = args.get<uint64_t>("skip_events");
  m_numEvents = args.get<uint64_t>("num_events");
}

std::string Application::outputPath(const std::string& name) const
{
  return m_outputPrefix + '-' + name;
}

Utils::EventLoop Application::makeEventLoop() const
{
  Utils::EventLoop loop(m_input.get(), m_skipEvents, m_numEvents);
  if (m_showProgress)
    loop.enableProgressBar();
  return loop;
}
