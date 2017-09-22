/**
 * \brief Convert from raw data files to the internal format
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-09
 */

#include "io/merger.h"
#include "io/rceroot.h"
#include "io/reader.h"
#include "utils/arguments.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

int main(int argc, char const* argv[])
{
  logger().setGlobalLevel(Utils::Logger::Level::Debug);

  Utils::Arguments args("convert data files to the internal format");
  args.addOption('s', "skip_events", "skip the first n events", 0);
  args.addOption('n', "num_events", "number of events to process", UINT64_MAX);
  args.addRequired("output", "path to the output file");
  args.addVariable("input", "path to the input file(s)");

  // parse prints help automatically
  if (args.parse(argc, argv))
    std::exit(EXIT_FAILURE);

  // open reader and writer
  std::vector<std::shared_ptr<Io::EventReader>> readers;
  for (const auto& path : args.get<std::vector<std::string>>("input"))
    readers.push_back(Io::openRead(path));
  auto merger = std::make_shared<Io::EventMerger>(readers);
  auto writer = std::make_shared<Io::RceRootWriter>(args.get("output"),
                                                    merger->numSensors());

  Utils::EventLoop loop(merger, merger->numSensors(),
                        args.get<uint64_t>("skip_events"),
                        args.get<uint64_t>("num_events"));
  loop.addWriter(writer);
  loop.run();

  return EXIT_SUCCESS;
}
