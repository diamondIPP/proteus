/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-10
 *
 * run analysis only
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <TFile.h>

#include "analyzers/eventprinter.h"
#include "mechanics/device.h"
#include "storage/storageio.h"
#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

int main(int argc, char const* argv[])
{
  using namespace Analyzers;

  Utils::DefaultArguments args("run proteus analysis");
  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  Utils::Logger::setGlobalLevel(Utils::Logger::Level::Debug);

  Mechanics::Device device = Mechanics::Device::fromFile(args.device());
  Storage::StorageIO input(args.input().c_str(), Storage::INPUT);
  TFile* hists = TFile::Open(args.makeOutput("hists.root").c_str(), "RECREATE");

  auto cfg = Utils::Config::readConfig(args.config());

  Utils::EventLoop loop(&input, args.skipEvents(), args.numEvents());
  loop.addAnalyzer(std::make_shared<EventPrinter>());
  loop.run();

  hists->Write();
  hists->Close();

  return EXIT_SUCCESS;
}
