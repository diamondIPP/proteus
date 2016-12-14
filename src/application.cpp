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
    : m_name(name), m_desc(description), m_cfg(defaults)
{
}

void Application::initialize(int argc, char const* argv[])
{
  Utils::Arguments args(m_desc);
  args.addOption('d', "device", "device configuration file", "device.toml");
  args.addOption('g', "geometry", "load a separate geometry file");
  args.addOption('c', "config", "analysis configuration file", "analysis.toml");
  args.addOption('u', "subsection", "use the given configuration sub-section");
  args.addOption('s', "skip_events", "skip the first n events", 0);
  args.addOption('n', "num_events", "number of events to process", -1);
  args.addRequiredArgument("input", "path to the input file");
  args.addRequiredArgument("output_prefix", "output path prefix");

  // should print help automatically
  if (args.parse(argc, argv))
    std::exit(EXIT_FAILURE);

  // TODO 2016-12-13 msmk: add command line flag
  Utils::Logger::setGlobalLevel(Utils::Logger::Level::Info);

  // read configuration w/ automatic handling of defaults
  auto pathCfg = args.option<std::string>("config");
  auto subsection = args.option<std::string>("subsection");
  std::string section =
      (subsection.empty() ? m_name : (m_name + '.' + subsection));
  auto cfgFull = Utils::Config::readConfig(pathCfg);
  // select configuration subsection
  const toml::Value* cfgTool = cfgFull.find(section);
  if (!cfgTool) {
    ERROR("configuration section '", m_name, "' is missing");
    std::exit(EXIT_FAILURE);
  }
  m_cfg = Utils::Config::withDefaults(*cfgTool, m_cfg);
  INFO("read configuration '", section, "' from '", pathCfg, "'");

  // read device w/ optional geometry override
  auto pathDev = args.option<std::string>("device");
  auto pathGeo = args.option<std::string>("geometry");
  m_dev.reset(new Mechanics::Device(Mechanics::Device::fromFile(pathDev)));
  if (!pathGeo.empty())
    m_dev->setGeometry(Mechanics::Geometry::fromFile(pathGeo));

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
