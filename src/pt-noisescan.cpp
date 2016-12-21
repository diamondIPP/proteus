/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-09
 *
 * run noise scan to find noisy pixels and create pixel masks
 */

#include <climits>
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

  // construct per-sensor configuration
  // clang-format off
  toml::Value defaults = toml::Table{
    {"density_bandwidth", 2.},
    {"max_sigma_above_avg", 5.},
    {"max_rate", 1.},
    {"col_min", INT_MIN},
    {"col_max", INT_MAX},
    {"row_min", INT_MIN},
    {"row_max", INT_MAX}};
  // clang-format on
  auto cfgAll = Utils::Config::readConfig(args.config());
  auto cfgTool = Utils::Config::withDefaults(cfgAll["noisescan"], defaults);
  std::vector<toml::Value> cfg =
      Utils::Config::perSensor(cfgTool, toml::Table());

  // sensor specific
  std::vector<std::shared_ptr<Analyzers::NoiseScan>> noiseScans;
  for (auto c = cfg.begin(); c != cfg.end(); ++c) {
    typedef Analyzers::NoiseScan::Area Area;
    typedef Analyzers::NoiseScan::Area::AxisInterval Interval;

    auto id = c->get<Index>("id");
    auto bandwidth = c->get<double>("density_bandwidth");
    auto sigmaMax = c->get<double>("max_sigma_above_avg");
    auto rateMax = c->get<double>("max_rate");
    // min/max are inclusive but Area uses right-open intervals
    Area roi(Interval(c->get<int>("col_min"), c->get<int>("col_max") + 1),
             Interval(c->get<int>("row_min"), c->get<int>("row_max") + 1));
    noiseScans.push_back(std::make_shared<Analyzers::NoiseScan>(
        device, id, bandwidth, sigmaMax, rateMax, roi, hists));
  }

  Utils::EventLoop loop(&input, args.get<uint64_t>("skip_events"),
                        args.get<uint64_t>("num_events"));
  loop.addAnalyzer(std::make_shared<Analyzers::Occupancy>(&device, hists));
  for (auto noise = noiseScans.begin(); noise != noiseScans.end(); ++noise)
    loop.addAnalyzer(*noise);
  loop.run();

  // store combined noise mask
  Mechanics::NoiseMask newMask;
  for (auto noise = noiseScans.begin(); noise != noiseScans.end(); ++noise)
    newMask.merge((*noise)->constructMask());
  newMask.writeFile(maskPath);

  hists->Write();
  hists->Close();

  return EXIT_SUCCESS;
}
