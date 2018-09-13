#include <algorithm>
#include <iterator>
#include <numeric>

#include <Compression.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TTree.h>

#include "alignment/correlationsaligner.h"
#include "alignment/residualsaligner.h"
#include "analyzers/correlations.h"
#include "analyzers/globaloccupancy.h"
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
  void write(const std::string& sensorName, TDirectory* dir) const
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
      sub->WriteTObject(g, nullptr, "Overwrite");
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

struct BeamStepsGraphs {
  std::vector<double> slope;
  std::vector<double> divergence;

  void addStep(double slope_, double divergence_)
  {
    slope.push_back(slope_);
    divergence.push_back(divergence_);
  }
  void write(const std::string& axis, TDirectory* dir) const
  {
    TGraphErrors* g =
        new TGraphErrors(std::min(slope.size(), divergence.size()));
    for (int i = 0; i < g->GetN(); ++i) {
      g->SetPoint(i, i, slope[i]);
      g->SetPointError(i, 0.0, divergence[i]);
    }
    g->SetName(("beam_slope_" + axis).c_str());
    g->SetTitle("");
    g->GetXaxis()->SetTitle("Alignment step");
    g->GetYaxis()->SetTitle(("Beam slope " + axis).c_str());
    dir->WriteTObject(g, nullptr, "Overwrite");
  }
};

struct StepsGraphs {
  std::map<Index, SensorStepsGraphs> sensors;
  BeamStepsGraphs beamX;
  BeamStepsGraphs beamY;

  void addStep(const std::vector<Index>& sensorIds,
               const Mechanics::Geometry& geo)
  {
    for (auto id : sensorIds) {
      sensors[id].addStep(geo.getParams(id), geo.getParamsCov(id));
    }
    auto dir = geo.beamDirection();
    auto div = geo.beamDivergence();
    beamX.addStep(dir[0] / dir[2], div[0]);
    beamY.addStep(dir[1] / dir[2], div[1]);
  }
  void write(const Mechanics::Device& device, TDirectory* dir) const
  {
    for (const auto& g : sensors) {
      g.second.write(device.getSensor(g.first).name(), dir);
    }
    beamX.write("x", dir);
    beamY.write("y", dir);
  }
};

void updateBeamParameters(const Analyzers::Tracks& tracks,
                          Mechanics::Geometry& geo)
{
  auto beamSlope = tracks.beamSlope();
  auto beamDivergence = tracks.beamDivergence();

  INFO("beam:");
  INFO("  slope x: ", beamSlope[0], " ± ", beamDivergence[0]);
  INFO("  slope y: ", beamSlope[1], " ± ", beamDivergence[1]);

  geo.setBeamSlope(beamSlope[0], beamSlope[1]);
  geo.setBeamDivergence(beamDivergence[0], beamDivergence[1]);
}

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
  TFile hists(app.outputPath("hists.root").c_str(), "RECREATE", "",
              ROOT::CompressionSettings(ROOT::kLZMA, 1));

  // alignment steps monitoring starting w/ the initial geometry
  StepsGraphs steps;
  steps.addStep(alignIds, dev.geometry());

  // iterative alignment steps
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
      loop.addAnalyzer(std::make_shared<Residuals>(stepDir, dev, sensorIds,
                                                   "unbiased_residuals"));
      tracks = std::make_shared<Tracks>(stepDir, dev);
      aligner =
          std::make_shared<ResidualsAligner>(stepDir, dev, alignIds, damping);

    } else {
      FAIL("unknown alignment method '", method, "''");
    }
    if (tracks) {
      loop.addAnalyzer(tracks);
    }
    loop.addAnalyzer(aligner);
    loop.run();

    // new geometry w/ updated sensor placement and beam parameters
    auto geo = aligner->updatedGeometry();
    if (tracks) {
      updateBeamParameters(*tracks, geo);
    }
    // update device for next iteration and write to prevent information loss
    dev.setGeometry(geo);
    dev.geometry().writeFile(app.outputPath("geo.toml"));

    // register updated geometry in alignment monitoring
    steps.addStep(alignIds, dev.geometry());
  }

  // validation step w/o geometry changes but final beam parameter updates
  {
    INFO("validation step ");

    TDirectory* subDir = Utils::makeDir(&hists, "validation");

    auto loop = app.makeEventLoop();

    // minimal processors for tracking
    setupHitPreprocessing(dev, loop);
    setupClusterizers(dev, loop);
    loop.addProcessor(std::make_shared<ApplyGeometry>(dev));
    loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
        dev, sensorIds, sensorIds.size(), searchSigmaMax, redChi2Max));
    loop.addProcessor(std::make_shared<Tracking::StraightFitter>(dev));

    // minimal set of analyzers
    loop.addAnalyzer(std::make_shared<GlobalOccupancy>(subDir, dev));
    // correlations analyzer does **not** sort an explicit list of sensors
    loop.addAnalyzer(std::make_shared<Correlations>(
        subDir, dev, Mechanics::sortedAlongBeam(dev.geometry(), sensorIds)));
    auto tracks = std::make_shared<Tracks>(subDir, dev);
    loop.addAnalyzer(tracks);
    loop.addAnalyzer(
        std::make_shared<Residuals>(subDir, dev, sensorIds, "residuals"));

    loop.run();

    // update beam parameters one more time using the final geometry
    auto geo = dev.geometry();
    updateBeamParameters(*tracks, geo);
    geo.writeFile(app.outputPath("geo.toml"));
    // no need to update the device; it will not be used again

    // register validation step in alignment monitoring
    steps.addStep(alignIds, geo);
  }

  steps.write(dev, Utils::makeDir(&hists, "summary"));
  hists.Write(nullptr, TFile::kOverwrite);
  hists.Close();

  return EXIT_SUCCESS;
}
