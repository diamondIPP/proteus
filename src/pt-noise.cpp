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

void run_noise(const std::string& input_path, const std::string& output_path)
{
  Storage::StorageIO input(input_path.c_str(), Storage::INPUT);
  Utils::EventLoop loop(&input);

  Processors::Processor* clusterer = new Processors::ClusterMaker(1, 1, 2);
  Analyzers::SingleAnalyzer* printer = new Analyzers::EventPrinter();

  loop.addProcessor(std::unique_ptr<Processors::Processor>(clusterer));
  loop.addAnalyzer(std::unique_ptr<Analyzers::SingleAnalyzer>(printer));

  Utils::globalLogger().setLevel(Utils::Logger::DEBUG);
  loop.run();
}

int main(int argc, char const* argv[])
{
  Utils::Arguments args("run proteus noise scan");
  if (args.parse(argc, argv))
    return EXIT_FAILURE;

  Utils::logger().setLevel(Utils::Logger::INFO);

  Mechanics::Device device = Mechanics::Device::fromFile(args.device());
  Storage::StorageIO input(args.input().c_str(), Storage::INPUT);
  TFile* hists = TFile::Open(args.makeOutput("hists.root").c_str(), "RECREATE");
  std::string maskPath = args.makeOutput("noise_mask.toml");

  auto cfg = Utils::Config::readConfig(args.config());
  auto noise =
      std::make_shared<Analyzers::NoiseScan>(device, cfg["noise_scan"], hists);

  Utils::EventLoop loop(&input);
  loop.addAnalyzer(std::make_shared<Analyzers::Occupancy>(&device, hists));
  loop.addAnalyzer(noise);
  loop.run();

  noise->writeMask(maskPath);
  hists->Write();
  hists->Close();

  return EXIT_SUCCESS;
}
