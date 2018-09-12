#include <algorithm>
#include <iterator>
#include <numeric>

#include <TFile.h>
#include <TGraphErrors.h>
#include <TTree.h>

#include "alignment/correlationsaligner.h"
#include "alignment/residualsaligner.h"
#include "analyzers/residuals.h"
#include "analyzers/tracks.h"
#include "loop/eventloop.h"
#include "mechanics/device.h"
#include "processors/applygeometry.h"
#include "processors/setupsensors.h"
#include "storage/event.h"
#include "tracking/straightfitter.h"
#include "tracking/trackfinder.h"
#include "utils/application.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(align)

/** Store sensor geometry parameters for multiple steps. */
struct SensorStepsGraphs {
  std::vector<double> offX;
  std::vector<double> offY;
  std::vector<double> offZ;
  std::vector<double> rotX;
  std::vector<double> rotY;
  std::vector<double> rotZ;
  std::vector<double> errOffX;
  std::vector<double> errOffY;
  std::vector<double> errOffZ;
  std::vector<double> errRotX;
  std::vector<double> errRotY;
  std::vector<double> errRotZ;

  void addStep(const Vector6& delta, const SymMatrix6& cov)
  {
    // values
    offX.push_back(delta[0]);
    offY.push_back(delta[1]);
    offZ.push_back(delta[2]);
    rotX.push_back(delta[3]);
    rotY.push_back(delta[4]);
    rotZ.push_back(delta[5]);
    // errors
    errOffX.push_back(std::sqrt(cov(0, 0)));
    errOffY.push_back(std::sqrt(cov(1, 1)));
    errOffZ.push_back(std::sqrt(cov(2, 2)));
    errRotX.push_back(std::sqrt(cov(3, 3)));
    errRotY.push_back(std::sqrt(cov(4, 4)));
    errRotZ.push_back(std::sqrt(cov(5, 5)));
  }
  void writeGraphs(const std::string& sensorName, TDirectory* dir) const
  {
    TDirectory* sub = Utils::makeDir(dir, sensorName);
    auto makeGraph = [&](const std::string& name, const std::string& ylabel,
                         const std::vector<double>& yval,
                         const std::vector<double>& yerr,
                         const double yscale = 1.0) {
      TGraphErrors* g = new TGraphErrors(std::min(yval.size(), yerr.size()));
      for (int i = 0; i < g->GetN(); ++i) {
        g->SetPoint(i, i, yscale * yval[i]);
        g->SetPointError(i, 0.0, yscale * yerr[i]);
      }
      g->SetName(name.c_str());
      g->SetTitle("");
      g->GetXaxis()->SetTitle("Alignment step");
      g->GetYaxis()->SetTitle((sensorName + ' ' + ylabel).c_str());
      sub->WriteTObject(g);
    };
    makeGraph("offset_x", "offset x", offX, errOffX);
    makeGraph("offset_y", "offset y", offY, errOffY);
    makeGraph("offset_z", "offset z", offZ, errOffZ);
    // show rotation angles in degree
    double yscale = 180.0 / M_PI;
    makeGraph("rotation_x", "rotation x / degree", rotX, errRotX, yscale);
    makeGraph("rotation_y", "rotation y / degree", rotY, errRotY, yscale);
    makeGraph("rotation_z", "rotation z / degree", rotZ, errRotZ, yscale);
  }
};

struct StepsGraphs {
  std::map<Index, SensorStepsGraphs> graphs;

  void addStep(const std::vector<Index>& sensorIds,
               const Mechanics::Geometry& geo)
  {
    for (auto id : sensorIds)
      graphs[id].addStep(geo.getParams(id), geo.getParamsCov(id));
  }
  void writeGraphs(const Mechanics::Device& device, TDirectory* dir) const
  {
    for (const auto& g : graphs)
      g.second.writeGraphs(device.getSensor(g.first)->name(), dir);
  }
};

int main(int argc, char const* argv[])
{
  using namespace Alignment;
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;

  toml::Table defaults = {{"num_steps", 1},
                          {"search_sigma_max", 5.},
                          {"reduced_chi2_max", -1.},
                          {"damping", 0.9}};
  Utils::Application app("align", "align selected sensors", defaults);
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");
  auto alignIds = app.config().get<std::vector<Index>>("align_ids");
  auto method = app.config().get<std::string>("method");
  auto numSteps = app.config().get<int>("num_steps");
  auto searchSigmaMax = app.config().get<double>("search_sigma_max");
  auto redChi2Max = app.config().get<double>("reduced_chi2_max");
  auto damping = app.config().get<double>("damping");

  // copy device to allow modifications after each alignment step
  auto dev = app.device();

  // split sensors into fixed and alignable set
  std::vector<Index> fixedSensorIds;
  std::sort(sensorIds.begin(), sensorIds.end());
  std::sort(alignIds.begin(), alignIds.end());
  std::set_difference(sensorIds.begin(), sensorIds.end(), alignIds.begin(),
                      alignIds.end(), std::back_inserter(fixedSensorIds));

  INFO("fixed sensors: ", fixedSensorIds);
  INFO("align sensors: ", alignIds);

  if (!std::includes(sensorIds.begin(), sensorIds.end(), alignIds.begin(),
                     alignIds.end())) {
    ERROR("align sensor set is not a subset of the input sensor set");
    return EXIT_FAILURE;
  }
  if (fixedSensorIds.empty()) {
    ERROR("no fixed sensors are given");
    return EXIT_FAILURE;
  }

  // output
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE");

  // alignment steps monitoring starting w/ the initial geometry
  StepsGraphs steps;
  steps.addStep(alignIds, dev.geometry());

  for (int step = 1; step <= numSteps; ++step) {
    TDirectory* stepDir = Utils::makeDir(&hists, "step" + std::to_string(step));

    INFO("alignment step ", step, "/", numSteps);

    // common event loop elements for all alignment methods
    auto loop = app.makeEventLoop();
    setupHitPreprocessing(dev, loop);
    setupClusterizers(dev, loop);
    loop.addProcessor(std::make_shared<ApplyGeometry>(dev));

    // setup aligment method specific loop logic
    std::shared_ptr<Tracks> tracks;
    std::shared_ptr<Aligner> aligner;
    if (method == "correlations") {
      // coarse method w/o tracks using only cluster correlations
      // use the first sensor that is not in the align set as reference
      aligner = std::make_shared<CorrelationsAligner>(
          stepDir, dev, fixedSensorIds.front(), alignIds);

    } else if (method == "residuals") {
      // use (unbiased) track residuals to align
      loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
          dev, sensorIds, sensorIds.size(), searchSigmaMax, redChi2Max));
      loop.addProcessor(
          std::make_shared<Tracking::UnbiasedStraightFitter>(dev));
      // add tracks analyzer for beam slope and divergence
      tracks = std::make_shared<Tracks>(stepDir, dev);
      loop.addAnalyzer(tracks);
      loop.addAnalyzer(std::make_shared<Residuals>(stepDir, dev, sensorIds,
                                                   "unbiased_residuals"));
      aligner =
          std::make_shared<ResidualsAligner>(stepDir, dev, alignIds, damping);
    } else {
      FAIL("unknown alignment method '", method, "''");
    }
    loop.addAnalyzer(aligner);
    loop.run();

    // updated geometry from the specified aligner
    Mechanics::Geometry newGeo = aligner->updatedGeometry();
    // update beam slope and divergence if available
    if (tracks) {
      auto beamSlope = tracks->beamSlope();
      auto beamDivergence = tracks->beamDivergence();

      INFO("beam:");
      INFO("  slope x: ", beamSlope[0]);
      INFO("  slope y: ", beamSlope[1]);
      INFO("  divergence x: ", beamDivergence[0]);
      INFO("  divergence y: ", beamDivergence[1]);

      newGeo.setBeamSlope(beamSlope[0], beamSlope[1]);
      newGeo.setBeamDivergence(beamDivergence[0], beamDivergence[1]);
    }
    // update device for next iteration and write to prevent information loss
    dev.setGeometry(newGeo);
    dev.geometry().writeFile(app.outputPath("geo.toml"));

    // register updated geometry in alignment monitoring
    steps.addStep(alignIds, dev.geometry());
  }

  steps.writeGraphs(dev, Utils::makeDir(&hists, "results"));
  hists.Write();
  hists.Close();

  return EXIT_SUCCESS;
}
