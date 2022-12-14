// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \brief Combine multiple raw data files into a single data stream
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-09
 */

#include "io/merger.h"
#include "io/open.h"
#include "io/rceroot.h"
#include "loop/eventloop.h"
#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/logger.h"

int main(int argc, char const* argv[])
{
  using namespace proteus;

  globalLogger().setMinimalLevel(Logger::Level::Warning);

  // To avoid having unused command line options, argument parsing is
  // implemented manually here w/ a limited amount of options compared to
  // the default Application.
  Arguments args("combine multiple data files into a single one");
  args.addOption('c', "config", "configuration file", "analysis.toml");
  args.addOption('u', "subsection", "use the given configuration sub-section");
  args.addOption('s', "skip_events", "skip the first n events", 0);
  args.addOption('n', "num_events", "number of events to process", UINT64_MAX);
  args.addFlag('q', "quiet", "print only errors");
  args.addFlag('v', "verbose", "print more information");
  args.addFlag('\0', "no-progress", "do not show a progress bar");
  args.addRequired("output", "path to the output file");
  args.addVariable("input", "path to the input file(s)");

  // parse prints help automatically
  if (args.parse(argc, argv))
    std::exit(EXIT_FAILURE);

  // logging level
  if (args.has("quiet")) {
    globalLogger().setMinimalLevel(Logger::Level::Warning);
  } else if (args.has("verbose")) {
    globalLogger().setMinimalLevel(Logger::Level::Verbose);
  } else {
    globalLogger().setMinimalLevel(Logger::Level::Info);
  }

  // read configuration file
  std::string section = "combine";
  if (args.has("subsection"))
    section += std::string(".") + args.get("subsection");
  const toml::Value cfgAll = configRead(args.get("config"));
  const toml::Value* cfg = cfgAll.find(section);
  if (!cfg)
    FAIL("configuration section '", section, "' is missing");
  INFO("read configuration '", section, "' from '", args.get("config"), "'");

  // open reader and writer
  std::vector<std::shared_ptr<Reader>> readers;
  for (const auto& path : args.get<std::vector<std::string>>("input"))
    readers.push_back(openRead(path, *cfg));
  auto merger = std::make_shared<EventMerger>(readers);
  auto writer =
      std::make_shared<RceRootWriter>(args.get("output"), merger->numSensors());

  EventLoop loop(merger, merger->numSensors(),
                 args.get<uint64_t>("skip_events"),
                 args.get<uint64_t>("num_events"), !args.has("no-progress"));
  loop.addWriter(writer);
  loop.run();

  return EXIT_SUCCESS;
}
