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

Application::Application(const std::string& name,
                         const std::string& description,
                         const toml::Value& defaults)
    : m_name(name), m_desc(description), m_cfg(defaults)
{
}

void Application::initialize(int argc, char const* argv[])
{
  Utils::Arguments args(m_desc);
  args.addOption('c', "config", "analysis configuration file", "analysis.toml");
  args.addOption('d', "device", "device configuration file", "device.toml");
  args.addOption('g', "geometry", "load a separate geometry file");
  args.addOption('s', "skip_events", "skip the first n events", 0);
  args.addOption('n', "num_events", "number of events to process", -1);
  args.addRequiredArgument("input", "path to the input file");
  args.addRequiredArgument("output_prefix", "output path prefix");

  // should print help automatically
  if (args.parse(argc, argv))
    return std::exit(EXIT_FAILURE);

  // TODO 2016-12-13 msmk: add command line flag
  Utils::Logger::setGlobalLevel(Utils::Logger::Level::Info);

  // read configuration w/ automatic handling of defaults
  auto cfgPath = args.option<std::string>("config");
  auto cfgFull = Utils::Config::readConfig(cfgPath);
  m_cfg = Utils::Config::withDefaults(cfgFull[m_name], m_cfg);
  // TODO 2016-12-13 msmk: allow config subsection

  // read device w/ optional geometry override
  auto devPath = args.option<std::string>("device");
  auto geoPath = args.option<std::string>("geometry");
  m_dev.reset(new Mechanics::Device(Mechanics::Device::fromFile(devPath)));
  if (!geoPath.empty())
    m_dev->setGeometry(Mechanics::Geometry::fromFile(geoPath));

  // setup input and i/o settings
  m_input.reset(new Storage::StorageIO(args.argument<std::string>(0),
                                       Storage::INPUT, m_dev->numSensors()));
  m_outputPrefix = args.argument<std::string>(1);
  m_skipEvents = args.option<uint64_t>("skip_events");
  m_numEvents = args.option<uint64_t>("num_events");
}

std::string Application::outputPath(const std::string& name) const
{
  return m_outputPrefix + '-' + name;
}

Utils::EventLoop Application::makeEventLoop() const
{
  return Utils::EventLoop(m_input.get(), m_skipEvents, m_numEvents);
}
