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
#include "application.h"
#include "io/rceroot.h"
#include "mechanics/device.h"
#include "utils/eventloop.h"
#include "utils/logger.h"

int main(int argc, char const* argv[])
{
  // clang-format off
  toml::Table defaults = {
    {"density_bandwidth", 2.},
    {"sigma_above_avg_max", 5.},
    {"rate_max", 1.},
    {"col_min", INT_MIN},
    // upper limit in the configuration is inclusive, but in the code the
    // interval is half-open w/ the upper limit being exclusive. Ensure +1 is
    // still within the numerical limits.
    {"col_max", INT_MAX - 1},
    {"row_min", INT_MIN},
    {"row_max", INT_MAX - 1}};
  // clang-format on
  Application app("noisescan", "run noise scan", defaults);
  app.initialize(argc, argv);

  // output
  TFile* hists = TFile::Open(app.outputPath("hists.root").c_str(), "RECREATE");

  // construct per-sensor configuration
  // construct per-sensor noise analyzer
  std::vector<toml::Value> cfg =
      Utils::Config::perSensor(app.config(), toml::Table());
  std::vector<std::shared_ptr<Analyzers::NoiseScan>> noiseScans;
  for (auto c = cfg.begin(); c != cfg.end(); ++c) {
    typedef Analyzers::NoiseScan::Area Area;
    typedef Analyzers::NoiseScan::Area::AxisInterval Interval;

    auto id = c->get<Index>("id");
    auto bandwidth = c->get<double>("density_bandwidth");
    auto sigmaMax = c->get<double>("sigma_above_avg_max");
    auto rateMax = c->get<double>("rate_max");
    // min/max are inclusive but Area uses right-open intervals
    Area roi(Interval(c->get<int>("col_min"), c->get<int>("col_max") + 1),
             Interval(c->get<int>("row_min"), c->get<int>("row_max") + 1));
    noiseScans.push_back(std::make_shared<Analyzers::NoiseScan>(
        *app.device().getSensor(id), bandwidth, sigmaMax, rateMax, roi, hists));
  }

  Utils::EventLoop loop = app.makeEventLoop();
  loop.addAnalyzer(
      std::make_shared<Analyzers::Occupancy>(&app.device(), hists));
  for (auto noise = noiseScans.begin(); noise != noiseScans.end(); ++noise)
    loop.addAnalyzer(*noise);
  loop.run();

  // store combined noise mask
  Mechanics::PixelMasks newMask;
  for (auto noise = noiseScans.begin(); noise != noiseScans.end(); ++noise)
    newMask.merge((*noise)->constructMasks());
  newMask.writeFile(app.outputPath("mask.toml"));

  hists->Write();
  hists->Close();

  return EXIT_SUCCESS;
}
