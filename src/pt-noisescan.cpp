/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-09
 *
 * run noise scan to find noisy pixels and create pixel masks
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <TFile.h>

#include "analyzers/noisescan.h"
#include "analyzers/occupancy.h"
#include "mechanics/device.h"
#include "storage/storageio.h"
#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

int main(int argc, char const* argv[])
{
  Utils::Arguments args("run proteus noise scan");
  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  Mechanics::Device device = Mechanics::Device::fromFile(args.device());
  Storage::StorageIO input(args.input().c_str(), Storage::INPUT);
  TFile* hists = TFile::Open(args.makeOutput("hists.root").c_str(), "RECREATE");
  std::string maskPath = args.makeOutput("mask.toml");

  auto cfg = Utils::Config::readConfig(args.config());
  auto noise =
      std::make_shared<Analyzers::NoiseScan>(device, cfg["noisescan"], hists);

  Utils::EventLoop loop(&input,
                        args.get<uint64_t>("skip_events"),
                        args.get<uint64_t>("num_events"));
  loop.addAnalyzer(std::make_shared<Analyzers::Occupancy>(&device, hists));
  loop.addAnalyzer(noise);
  loop.run();

  noise->constructMask().writeFile(maskPath);
  hists->Write();
  hists->Close();

  return EXIT_SUCCESS;
}
