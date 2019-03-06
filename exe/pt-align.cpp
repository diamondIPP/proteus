#include <algorithm>
#include <iterator>
#include <numeric>

#include <Compression.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TTree.h>

#include "alignment/correlationsaligner.h"
#include "alignment/localchi2aligner.h"
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

/** Store values for every step and produce a graph at the end. */
struct StepsGraph {
  std::vector<double> values;
  std::vector<double> errors;

  void addStep(double value, double error = 0.0)
  {
    values.push_back(value);
    errors.push_back(error);
  }
  void write(const std::string& name,
             const std::string& ylabel,
             TDirectory* dir) const
  {
    TGraphErrors* g = new TGraphErrors(std::min(values.size(), errors.size()));
    for (int i = 0; i < g->GetN(); ++i) {
      g->SetPoint(i, i, values[i]);
      g->SetPointError(i, 0.0, errors[i]);
    }
    g->SetName(name.c_str());
    g->SetTitle("");
    g->GetXaxis()->SetTitle("Alignment step");
    g->GetYaxis()->SetTitle(ylabel.c_str());
    dir->WriteTObject(g, nullptr, "Overwrite");
  }
};

/** Store sensor geometry parameters for multiple steps. */
struct SensorStepsGraphs {
  StepsGraph x;
  StepsGraph y;
  StepsGraph z;
  StepsGraph alpha;
  StepsGraph beta;
  StepsGraph gamma;

  void addStep(const Vector6& params, const SymMatrix6& cov)
  {
    Vector6 stdev = extractStdev(cov);
    x.addStep(params[0], stdev[0]);
    y.addStep(params[1], stdev[1]);
    z.addStep(params[2], stdev[2]);
    alpha.addStep(degree(params[3]), degree(stdev[3]));
    beta.addStep(degree(params[4]), degree(stdev[3]));
    gamma.addStep(degree(params[5]), degree(stdev[3]));
  }
  void write(TDirectory* dir) const
  {
    x.write("offset_x", "Offset x", dir);
    y.write("offset_y", "Offset y", dir);
    z.write("offset_z", "Offset z", dir);
    alpha.write("rotation_alpha", "Rotation #alpha / #circ", dir);
    beta.write("rotation_beta", "Rotation #beta / #circ", dir);
    gamma.write("rotation_gamma", "Rotation #gamma / #circ", dir);
  }
};

struct StepsGraphs {
  std::map<Index, SensorStepsGraphs> sensors;
  StepsGraph beamX;
  StepsGraph beamY;
  StepsGraph numTracks;

  void addStep(const std::vector<Index>& sensorIds,
               const Mechanics::Geometry& geo,
               double ntracks = 0.0)
  {
    for (auto id : sensorIds) {
      sensors[id].addStep(geo.getParams(id), geo.getParamsCov(id));
    }
    auto slope = geo.beamSlope();
    auto slopeStdev = extractStdev(geo.beamSlopeCovariance());
    beamX.addStep(slope[0], slopeStdev[0]);
    beamY.addStep(slope[1], slopeStdev[1]);
    numTracks.addStep(ntracks);
  }
  void write(const Mechanics::Device& device, TDirectory* dir) const
  {
    for (const auto& g : sensors) {
      g.second.write(Utils::makeDir(dir, device.getSensor(g.first).name()));
    }
    beamX.write("beam_slope_x", "Beam slope x", dir);
    beamY.write("beam_slope_y", "Beam slope y", dir);
    numTracks.write("ntracks", "Tracks / event", dir);
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

  geo.setBeamSlope(beamSlope);
  geo.setBeamDivergence(beamDivergence);
}

int main(int argc, char const* argv[])
{
  using namespace Alignment;
  using namespace Analyzers;
  using namespace Mechanics;
  using namespace Processors;
  using namespace Tracking;

  toml::Table defaults = {
      // tracking settings
      {"search_sigma_max", 5.},
      {"reduced_chi2_max", -1.},
      // alignment settings
      {"num_steps", 1},
      {"damping", 0.9},
      {"estimate_beam_parameters", true},
  };
  Utils::Application app("align", "align selected sensors", defaults);
  app.initialize(argc, argv);

  // configuration
  auto sensorIds = app.config().get<std::vector<Index>>("sensor_ids");
  auto alignIds = app.config().get<std::vector<Index>>("align_ids");
  auto method = app.config().get<std::string>("method");
  auto numSteps = app.config().get<int>("num_steps");
  auto damping = app.config().get<double>("damping");
  auto estimateBeamParameters =
      app.config().get<bool>("estimate_beam_parameters");
  auto searchSigmaMax = app.config().get<double>("search_sigma_max");
  auto redChi2Max = app.config().get<double>("reduced_chi2_max");

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
  auto hists = Utils::openRootWrite(app.outputPath("hists.root"));

  // copy device to allow modifications after each alignment step
  auto dev = app.device();

  // alignment steps monitoring starting w/ the initial geometry
  StepsGraphs steps;
  steps.addStep(alignIds, dev.geometry());

  // iterative alignment steps
  for (int step = 1; step <= numSteps; ++step) {
    TDirectory* stepDir =
        Utils::makeDir(hists.get(), "step" + std::to_string(step));

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
      // use unbiased track residuals to align. this means we need a specific
      // track fitter and should not use the automatic fitter selection.
      loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
          dev, sensorIds, sensorIds.size(), searchSigmaMax, redChi2Max));
      loop.addProcessor(
          std::make_shared<Tracking::UnbiasedStraight3dFitter>(dev));
      loop.addAnalyzer(std::make_shared<Residuals>(stepDir, dev, sensorIds,
                                                   "unbiased_residuals"));
      tracks = std::make_shared<Tracks>(stepDir, dev);
      aligner =
          std::make_shared<ResidualsAligner>(stepDir, dev, alignIds, damping);

    } else if (method == "localchi2") {
      // use unbiased track residuals to align. this means we need a specific
      // track fitter and should not use the automatic fitter selection.
      loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
          dev, sensorIds, sensorIds.size(), searchSigmaMax, redChi2Max));
      loop.addProcessor(
          std::make_shared<Tracking::UnbiasedStraight3dFitter>(dev));
      loop.addAnalyzer(std::make_shared<Residuals>(stepDir, dev, sensorIds,
                                                   "unbiased_residuals"));
      tracks = std::make_shared<Tracks>(stepDir, dev);
      aligner = std::make_shared<LocalChi2Aligner>(dev, alignIds, damping);

    } else {
      FAIL("unknown alignment method '", method, "'");
    }
    if (tracks) {
      loop.addAnalyzer(tracks);
    }
    loop.addAnalyzer(aligner);
    loop.run();

    // new geometry w/ updated sensor placement and beam parameters (optional)
    auto geo = aligner->updatedGeometry();
    if (estimateBeamParameters and tracks) {
      updateBeamParameters(*tracks, geo);
    }
    // update device for next iteration and write to prevent information loss
    dev.setGeometry(geo);
    dev.geometry().writeFile(app.outputPath("geo.toml"));

    // register updated geometry in alignment monitoring
    double ntracks = (tracks ? tracks->avgNumTracks() : 0.0);
    steps.addStep(alignIds, dev.geometry(), ntracks);
  }

  // validation step w/o geometry changes but final beam parameter updates
  {
    INFO("validation step");

    TDirectory* subDir = Utils::makeDir(hists.get(), "validation");

    auto loop = app.makeEventLoop();

    // minimal processors for tracking
    setupHitPreprocessing(dev, loop);
    setupClusterizers(dev, loop);
    loop.addProcessor(std::make_shared<ApplyGeometry>(dev));
    loop.addProcessor(std::make_shared<Tracking::TrackFinder>(
        dev, sensorIds, sensorIds.size(), searchSigmaMax, redChi2Max));
    loop.addProcessor(
        std::make_shared<Tracking::UnbiasedStraight3dFitter>(dev));

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

    auto geo = dev.geometry();
    // update beam parameters one more time using the final geometry (optional)
    if (estimateBeamParameters) {
      updateBeamParameters(*tracks, geo);
      geo.writeFile(app.outputPath("geo.toml"));
      // no need to update the device; it will not be used again
    }
    // close alignment monitoring w/ the final validation step geometry
    steps.addStep(alignIds, geo, tracks->avgNumTracks());
  }
  steps.write(dev, Utils::makeDir(hists.get(), "summary"));

  return EXIT_SUCCESS;
}
