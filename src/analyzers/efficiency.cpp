#include "efficiency.h"
#include <algorithm>    // std::sort
#include <cassert>
#include <sstream>
#include <math.h>
#include <vector>
#include <float.h>

#include <TDirectory.h>
#include <TH2D.h>
#include <TH1D.h>
#include <TError.h>
#include <TMath.h>

// Access to the device being analyzed and its sensors
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
// Access to the data stored in the event
#include "../storage/hit.h"
#include "../storage/cluster.h"
#include "../storage/plane.h"
#include "../storage/track.h"
#include "../storage/event.h"
// Some generic processors to calcualte typical event related things
#include "../processors/processors.h"
// This header defines all the cuts
#include "cuts.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;

//=========================================================
Analyzers::Efficiency::Efficiency(const Mechanics::Device *refDevice,
                                  const Mechanics::Device *dutDevice,
                                  TDirectory *dir,
                                  const char *suffix,
                                  int relativeToSensor,
                                  int pix_x_min,
                                  int pix_x_max,
                                  int pix_y_min,
                                  int pix_y_max,
                                  int min_entries_lvl1,
                                  int min_entries_lvl1_matchedTracks,
                                  unsigned int rebinX,
                                  unsigned int rebinY,
                                  unsigned int pixBinsX,
                                  unsigned int pixBinsY) :
  DualAnalyzer(refDevice, dutDevice, dir, suffix),
  _relativeToSensor(relativeToSensor),
  _pix_x_min(pix_x_min),
  _pix_x_max(pix_x_max),
  _pix_y_min(pix_y_min),
  _pix_y_max(pix_y_max),
  _min_entries_lvl1(min_entries_lvl1),
  _min_entries_lvl1_matchedTracks(min_entries_lvl1_matchedTracks)
{
  assert(refDevice && dutDevice && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory *plotDir_Efficiency = makeGetDirectory("Efficiency");
  TDirectory *plotDir_Timing = makeGetDirectory("Timing_Efficiency");
  TDirectory *plotDir_ClusterEfficiency = makeGetDirectory("Cluster_Efficiency");
  TDirectory *plotDir_PhaseTrigger = makeGetDirectory("PhaseTrigger");


  std::stringstream name; // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo
  std::stringstream Xaxis;

  if (_relativeToSensor >= (int)_dutDevice->getNumSensors())
    throw "Efficiency: relative sensor exceeds range";

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _dutDevice->getNumSensors(); nsens++) {
    const Mechanics::Sensor *sensor = _dutDevice->getSensor(nsens);

    unsigned int nx = sensor->getPosNumX() / rebinX;
    unsigned int ny = sensor->getPosNumY() / rebinY;
    std::cout << "nx " << sensor->getNumX() << " rebinX " << rebinX << " = " << sensor->getNumX() / rebinX << std::endl;
    std::cout << "ny " << sensor->getNumY() << " rebinY " << rebinY << " = " << sensor->getNumY() / rebinY << std::endl;
    std::cout << "nx " << sensor->getPosNumX() << " rebinX " << rebinX << " = " << sensor->getPosNumX() / rebinX << std::endl;
    std::cout << "ny " << sensor->getPosNumY() << " rebinY " << rebinY << " = " << sensor->getPosNumY() / rebinY << std::endl;

    if (nx < 1) nx = 1;
    if (ny < 1) ny = 1;
    const double lowX = sensor->getOffX() - sensor->getPosSensitiveX() / 2 ; // sensor->getSensitiveX() cambiare pitch sul config file
    const double uppX = sensor->getOffX() + sensor->getPosSensitiveX() / 2;
    const double lowY = sensor->getOffY() - sensor->getPosSensitiveY() / 2 ;
    const double uppY = sensor->getOffY() + sensor->getPosSensitiveY() / 2 ;

    std::cout << "offset x " << sensor->getOffX() << " sensor getSensitive x " << sensor->getPosSensitiveX() / 2 << "= " << lowX << std::endl;
    std::cout << "offset x " << sensor->getOffX() << " sensor getSensitive x " << sensor->getPosSensitiveX() / 2 << "= " << uppX << std::endl;
    std::cout << "offset y " << sensor->getOffY() << " sensor getSensitive y " << sensor->getPosSensitiveY() / 2 << "= " << lowY << std::endl;
    std::cout << "offset y " << sensor->getOffY() << " sensor getSensitive y " << sensor->getPosSensitiveY() / 2 << "= " << uppY << std::endl;

    // TODO: change to pixel space

    // Efficiency map initialization
    name.str(""); title.str("");
    name<< sensor->getName() << "Map" << _nameSuffix;
    title << sensor->getName()
          << " [Efficiency Map]"
          << ";X position" << " [" << _dutDevice->getSpaceUnit() << "]"
          << ";Y position" << " [" << _dutDevice->getSpaceUnit() << "]"
          << ";Efficiency";
    TEfficiency *map = new TEfficiency(name.str().c_str(), title.str().c_str(),
                                       nx, lowX, uppX,
                                       ny, lowY, uppY);

    cout << "Pixel X from " << _pix_x_min << " to " << _pix_x_max << endl;
    cout << "Pixel Y from " << _pix_y_min << " to " << _pix_y_max << endl;
    //-----------------------------------------------------------
    _maxclustersize = 5;
    //-----------------------------------------------------------



//---------Residual plot
      name.str(""); title.str("");
      name <<
            "ResX";
      title << "ResX ";
      TH1D* resX1d = new TH1D(name.str().c_str(), title.str().c_str(),
                             200,-400, 400);
      resX1d->SetDirectory(plotDir_Efficiency);
      _ResX.push_back(resX1d);



      name.str(""); title.str("");
      name  <<  "ResY";
      title << "ResY";
      TH1D* resY1d = new TH1D(name.str().c_str(), title.str().c_str(),
                             500,-1000, 1000);
     //                        50,-5, 5);
      resY1d->SetDirectory(plotDir_Efficiency);
      _ResY.push_back(resY1d);






    map->SetDirectory(plotDir_Efficiency);
    map->SetStatisticOption(TEfficiency::kFWilson);
    _efficiencyMap.push_back(map);

    // Pixel grouped efficieny initialization
    name.str(""); title.str("");
    name << sensor->getName()
         << "GroupedDistribution" << _nameSuffix;
    title << sensor->getName()
          << " [Grouped Efficiency]";
    TH1D *pixels = new TH1D(name.str().c_str(), title.str().c_str(),
                            20, 0, 1.001);
    pixels->GetXaxis()->SetTitle("Pixel group efficiency");
    pixels->GetYaxis()->SetTitle("Pixel groups / 0.05");
    pixels->SetDirectory(plotDir_Efficiency);
    _efficiencyDistribution.push_back(pixels);

    // Track matched position (X)
    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getName()
         << "MatchedTracksPositionX" << _nameSuffix;
    title << sensor->getName()
          << " [Matched tracks : Position-X]";
    TH1D *matchedPositionX = new TH1D(name.str().c_str(), title.str().c_str(),
                //                      sensor->getNumX() * 1.5 * 3, 1.5 * lowX, 1.5 * uppX);
                                      200, 0, 100);
    Xaxis << "X-position" << " [" << _dutDevice->getSpaceUnit() << "]";
    matchedPositionX->GetXaxis()->SetTitle(Xaxis.str().c_str());
    matchedPositionX->SetDirectory(plotDir_Efficiency);
    _matchPositionX.push_back(matchedPositionX);

    // Track matched position (Y)
    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getName()
         << "MatchedTracksPositionY" << _nameSuffix;
    title << sensor->getName()
          << " [Matched tracks : Position-Y]";
    TH1D *matchedPositionY = new TH1D(name.str().c_str(), title.str().c_str(),
                                      sensor->getNumY() * 1.5 * 3, 1.5 * lowY, 1.5 * uppY);
    Xaxis << "Y-position" << " [" << _dutDevice->getSpaceUnit() << "]";
    matchedPositionY->GetXaxis()->SetTitle(Xaxis.str().c_str());
    matchedPositionY->SetDirectory(plotDir_Efficiency);
    _matchPositionY.push_back(matchedPositionY);

    //-----------------------------------------------------------
    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getName() << "MatchedTiming" << _nameSuffix;
    title << sensor->getName()
          << " [Matched tracks : Timing]";
    TH1D *matchTiming = new TH1D(name.str().c_str(), title.str().c_str(),
                                 16, 0, 16);
    matchTiming->GetXaxis()->SetTitle("Timing [BC]");
    matchTiming->GetYaxis()->SetTitle("# hits");
    matchTiming->SetDirectory(plotDir_Efficiency);
    _matchTime.push_back(matchTiming);
    //-----------------------------------------------------------
    // DUT TDC start
    std::vector<TH1D *> tmp700;
    name.str(""); title.str(""); Xaxis.str("");
    for (int BC = 0; BC <= 16; BC++) {
      name.str(""); title.str("");
      name << sensor->getName() << "TDC_DUT_BC_" << BC << _nameSuffix;
      title << sensor->getName() << " [Matched tracks : TDC_DUT_BC_" << BC << "]";
      TH1D *triggerPhaseDUT = new TH1D(name.str().c_str(), title.str().c_str(), 65, -0.5, 63.5);
      triggerPhaseDUT->GetXaxis()->SetTitle("TriggerPhase");
      triggerPhaseDUT->GetYaxis()->SetTitle("# hits");
      triggerPhaseDUT->SetDirectory(plotDir_PhaseTrigger);
      tmp700.push_back(triggerPhaseDUT);
    }
    _triggerPhaseDUT.push_back(tmp700);
//TDC Altri piani

    std::vector<TH1D *> tmp800;
    name.str(""); title.str(""); Xaxis.str("");
    for (int NUM_PLANE = 0; NUM_PLANE < 6; NUM_PLANE++) {
      for (int BC = 0; BC < 4; BC++) {
        name.str(""); title.str("");
        name << sensor->getName() << "TDC_PLANE_" << NUM_PLANE << "_BC_" << BC << _nameSuffix;
        title << sensor->getName() << " [Matched tracks : TDC_PLANE_" << NUM_PLANE << "_BC_" << BC << "]";
        TH1D *triggerPhaseREF = new TH1D(name.str().c_str(), title.str().c_str(), 65, -0.5, 63.5);
        triggerPhaseREF->GetXaxis()->SetTitle("TriggerPhase");
        triggerPhaseREF->GetYaxis()->SetTitle("# hits");
        triggerPhaseREF->SetDirectory(plotDir_PhaseTrigger);
        tmp800.push_back(triggerPhaseREF);
      }
      _triggerPhaseREF.push_back(tmp800);
      tmp800.clear();
    }
/*
    name.str(""); title.str(""); Xaxis.str("");
    for (int NUM_PLANE = 0; NUM_PLANE < 6; NUM_PLANE++) {
      name.str(""); title.str("");
      name << sensor->getName() << "_LVL1TriggerREF_" << NUM_PLANE <<"_"<<  _nameSuffix;
      title << " - LVL1 - " << sensor->getName() << " [Matched tracks : Timing]";
      TH1D *matchTiming_LV1_REF = new TH1D(name.str().c_str(), title.str().c_str(),5, -0.5, 4.5);
      matchTiming_LV1_REF->GetXaxis()->SetTitle("Timing [BC]");
      matchTiming_LV1_REF->GetYaxis()->SetTitle("# hits");
      matchTiming_LV1_REF->SetDirectory(plotDir_PhaseTrigger);
      _LVL1TriggerREF.push_back(matchTiming_LV1_REF);
    }
    name.str(""); title.str("");
    */
    /*
        std::vector<TH1D *> tmp7;
        for (int cl_size = 0; cl_size <= _maxclustersize; cl_size++) {
          name.str(""); title.str("");
          name << sensor->getName()
               << Form("Timing_Cluster_%d", cl_size+1) << _nameSuffix;
          title << " " << sensor->getName()
                << Form("Timing for cluster = %d ", cl_size+1);
          TH1D *timing_cluster = new TH1D(name.str().c_str(), title.str().c_str(),16, 0, 16);
          timing_cluster->SetDirectory(plotDir_ClusterEfficiency);
          tmp7.push_back(timing_cluster);
        }
        _TimingCluster.push_back(tmp7);
        tmp7.clear();



    */

    //DUT TDC END
    //-----------------------------------------------------------



    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getName() << "Matchedcluster" << _nameSuffix;
    title << sensor->getName()
          << " [Matched tracks : Cluster]";
    TH1D *matchCluster = new TH1D(name.str().c_str(), title.str().c_str(),
                                  10, 0, 10);
    matchCluster->GetXaxis()->SetTitle("Cluster Size");
    matchCluster->GetYaxis()->SetTitle("# hits");
    matchCluster->SetDirectory(plotDir_Efficiency);
    _matchCluster.push_back(matchCluster);

    //-----------------------------------------------------------

    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getName() << "Matched_ToT" << _nameSuffix;
    title << sensor->getName() << " [ToT distribution]";
    TH1D *matchToT = new TH1D(name.str().c_str(), title.str().c_str(),
                              18, 0, 18);
    matchToT->GetXaxis()->SetTitle("ToT [BC]");
    matchToT->SetDirectory(plotDir_Efficiency);
    _matchToT.push_back(matchToT);

    //-----------------------------------------------------------
    name.str(""); title.str("");
    name << sensor->getName()
         << "lvl1_vs_ToT" << _nameSuffix;
    title << sensor->getName()
          << " [Timing vs ToT]";
    TH2D *lvl1vsToT = new TH2D(name.str().c_str(), title.str().c_str(),
                               16, 0, 16, // LVL1
                               18, 0, 18); // ToT
    lvl1vsToT->GetXaxis()->SetTitle("LVL1 [BC]");
    lvl1vsToT->GetYaxis()->SetTitle("ToT [BC]");
    lvl1vsToT->SetDirectory(plotDir_Efficiency);
    _lvl1vsToT.push_back(lvl1vsToT);
    //-----------------------------------------------------------
    std::vector<TH2D *> tmp4;
    for (int cl_size = 0; cl_size <= _maxclustersize; cl_size++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("TimingTOT_cl_%d", cl_size + 1) << _nameSuffix;
      title << sensor->getName()
            << Form(" Timing vs TOT for cluster size = %d", cl_size + 1);
      TH2D *TimingTOT_cluster = new TH2D(name.str().c_str(), title.str().c_str(),
                                         16, 0, 16, // LVL1
                                         18, 0, 18); // ToT
      TimingTOT_cluster->GetXaxis()->SetTitle("LVL1 [BC]");
      TimingTOT_cluster->GetYaxis()->SetTitle("ToT [BC]");
      TimingTOT_cluster->SetDirectory(plotDir_ClusterEfficiency);
      tmp4.push_back(TimingTOT_cluster);
    }
    _TimingTOT_cluster.push_back(tmp4);
    tmp4.clear();
    //-----------------------------------------------------------

    name.str(""); title.str("");
    name << sensor->getName()
         << "InPixelTiming" << _nameSuffix;
    title << sensor->getName()
          << " [InPixel Timing Map]";
    TH2D *inTimingMap = new TH2D(name.str().c_str(), title.str().c_str(),
                                 pixBinsX, 0, sensor->getPitchX(),
                                 pixBinsY, 0, sensor->getPitchY());
    inTimingMap->SetDirectory(plotDir_Efficiency);
    _inPixelTiming.push_back(inTimingMap);

    //==================================================


    std::vector<TH2D *> tmp;
    for (int bc = 0; bc < 16; bc++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("InPixelTiming_bc_%d", bc) << _nameSuffix;
      title << sensor->getName()
            << Form(" InPixel Timing Map for lvl= %d bc", bc);
      TH2D *inTimingMap1 = new TH2D(name.str().c_str(), title.str().c_str(),
                                    pixBinsX, 0, sensor->getPitchX(),
                                    pixBinsY, 0, sensor->getPitchY());
      inTimingMap1->SetDirectory(plotDir_Timing);
      tmp.push_back(inTimingMap1);
    }
    _inPixelTimingLVL1.push_back(tmp);
    tmp.clear();


    // InPixel efficiency : all LVL1
    name.str(""); title.str("");
    name << sensor->getName()
         << "total_cluster" << _nameSuffix;
    title << sensor->getName()
          << " InPixel Efficiency Map for lvl=  bc";
    TH2D *inPixel_LVL1_total = new TH2D(name.str().c_str(), title.str().c_str(),
                                        pixBinsX, 0, sensor->getPitchX(),
                                        pixBinsY, 0, sensor->getPitchY());
    inPixel_LVL1_total->SetDirectory(0);
    _inPixelEfficiencyLVL1_total.push_back(inPixel_LVL1_total);


    //-----------------------------------------------------------
    std::vector<TH2D *> tmp2;
    for (int bc = 0; bc < 16; bc++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("InPixelEfficiencypa_bc_%d", bc) << _nameSuffix;
      title << sensor->getName()
            << Form(" In Pixel Efficiency Map for lvl= %d bc", bc);
      TH2D *inPixel_LVL1_passed = new TH2D(name.str().c_str(), title.str().c_str(),
                                           pixBinsX, 0, sensor->getPitchX(),
                                           pixBinsY, 0, sensor->getPitchY());
      inPixel_LVL1_passed->SetDirectory(plotDir_Timing);
      tmp2.push_back(inPixel_LVL1_passed);
    }
    _inPixelEfficiencyLVL1_passed.push_back(tmp2);
    tmp2.clear();

    //telescpe correlation_plot
    std::vector<TH2D *> tmppp;
    for (int plane = 0; plane < 6; plane++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("Correlation_plot_plane_%d_%d", plane,plane+1) << _nameSuffix;
      title << sensor->getName()
            << Form(" Correlation plane %d and %d", plane, plane+1);
      TH2D *correlation_plane = new TH2D(name.str().c_str(), title.str().c_str(),
                                           16, 0, 16,
                                           16, 0, 16);
      correlation_plane->SetDirectory(plotDir_ClusterEfficiency);
      tmppp.push_back(correlation_plane);
    }
    _TimingCorr.push_back(tmppp);
    tmppp.clear();



    std::vector<TH1D *> tmpp;
    for (int plane = 0; plane < 6; plane++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("Timing_Plane_%d", plane) << _nameSuffix;
      title << sensor->getName()
            << Form("Timing distribution plane= %d ", plane);
      TH1D *correlation_plane = new TH1D(name.str().c_str(), title.str().c_str(),
                                           16, 0, 16);
      correlation_plane->SetDirectory(plotDir_ClusterEfficiency);
      tmpp.push_back(correlation_plane);
    }
    _TimingPlot.push_back(tmpp);
    tmpp.clear();







      name.str(""); title.str("");
      name << sensor->getName()
           << "Timing_diff" << _nameSuffix;
      title << sensor->getName()
            << "Timing_diff"<<_nameSuffix;
      TH1D *TimeDif = new TH1D(name.str().c_str(), title.str().c_str(),
                                           16, 0, 16);
     TimeDif->SetDirectory(plotDir_Efficiency);
    _timing_diff.push_back(TimeDif);









    std::vector<TH1D *> tmp7;
    for (int cl_size = 0; cl_size <= _maxclustersize; cl_size++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("Timing_Cluster_%d", cl_size + 1) << _nameSuffix;
      title << sensor->getName()
            << Form("Timing for cluster = %d ", cl_size+1);
      TH1D *timing_cluster = new TH1D(name.str().c_str(), title.str().c_str(),16, 0, 16);
      timing_cluster->SetDirectory(plotDir_ClusterEfficiency);
      tmp7.push_back(timing_cluster);
    }
    _TimingCluster.push_back(tmp7);
    tmp7.clear();

    //std::vector<TH1D *> tmp7;
    for (int cl_size = 0; cl_size <= _maxclustersize; cl_size++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("_Fast_Timing_Cluster_%d", cl_size+1) << _nameSuffix;
      title << sensor->getName()
            << Form("Fast Timing for cluster = %d ", cl_size+1);
      TH1D *timing_cluster_fast = new TH1D(name.str().c_str(), title.str().c_str(),16, 0, 16);
      timing_cluster_fast->SetDirectory(plotDir_ClusterEfficiency);
      tmp7.push_back(timing_cluster_fast);
    }
    _TimingCluster_fast.push_back(tmp7);
    tmp7.clear();
    //std::vector<TH1D *> tmp7;
    for (int cl_size = 0; cl_size <= _maxclustersize; cl_size++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("_Fast_Timing_Cluster_%d", cl_size + 1) << _nameSuffix;
      title << sensor->getName()
            << Form("Fast Timing for cluster = %d ", cl_size + 1);
      TH1D *timing_cluster_fast = new TH1D(name.str().c_str(), title.str().c_str(), 16, 0, 16);
      timing_cluster_fast->SetDirectory(plotDir_ClusterEfficiency);
      tmp7.push_back(timing_cluster_fast);
    }
    _TimingCluster_fast.push_back(tmp7);
    tmp7.clear();

    std::vector<TH1D *> tmp9;
    for (int cl_size = 0; cl_size <= _maxclustersize; cl_size++) {
      name.str(""); title.str("");
      name << sensor->getName()
           << Form("_Slow_Timing_Cluster_%d", cl_size+1) << _nameSuffix;
      title << sensor->getName()
            << Form("Slow Timing for cluster = %d ", cl_size+1);
      TH1D *timing_cluster_slow = new TH1D(name.str().c_str(), title.str().c_str(),16, 0, 16);
      timing_cluster_slow->SetDirectory(plotDir_ClusterEfficiency);
      tmp9.push_back(timing_cluster_slow);
    }
    _TimingCluster_slow.push_back(tmp9);
    tmp9.clear();



    // Clusterefficiency
    //-----------------------------------------------------------

    std::vector<TH2D *> tmp5;
    for (int cl_size = 0; cl_size < 1; cl_size++) {
      name.str(""); title.str("");
      name<< sensor->getName()
           << Form("InPixelEfficiencyClusterMap_%d", cl_size) << _nameSuffix;
      title << sensor->getName()
            << Form("Total Cluster Efficiency Map for cluster = %d ", cl_size);
      TH2D *cluster_total = new TH2D(name.str().c_str(), title.str().c_str(),
                                     pixBinsX, 0, sensor->getPitchX(),
                                     pixBinsY, 0, sensor->getPitchY());
      cluster_total->SetDirectory(plotDir_ClusterEfficiency);
      tmp5.push_back(cluster_total);
    }
    //cout<<"Ciao"<<endl;
    _cluster_total.push_back(tmp5);
    tmp5.clear();


    std::vector<TH2D *> tmp3;
    for (int cl_size = 0; cl_size <= _maxclustersize; cl_size++) {
      name.str(""); title.str("");
      name<< sensor->getName()
           << Form("InPixelEfficiencyCluster_%d", cl_size + 1) << _nameSuffix;
      title << sensor->getName()
            << Form(" Cluster Efficiency Map for cluster = %d ", cl_size + 1);
      TH2D *cluster_passed = new TH2D(name.str().c_str(), title.str().c_str(),
                                      pixBinsX, 0, sensor->getPitchX(),
                                      pixBinsY, 0, sensor->getPitchY());
      cluster_passed->SetDirectory(plotDir_ClusterEfficiency);
      tmp3.push_back(cluster_passed);
    }
    _cluster_passed.push_back(tmp3);
    tmp3.clear();

    //-----------------------------------------------------------


    name.str(""); title.str("");
    name<< sensor->getName()
         << "InPixelToT" << _nameSuffix;
    title << sensor->getName()
          << " In Pixel ToT Map";
    TH2D *inToTMap = new TH2D(name.str().c_str(), title.str().c_str(),
                              pixBinsX, 0, sensor->getPitchX(),
                              pixBinsY, 0, sensor->getPitchY());
    inToTMap->SetDirectory(plotDir_Efficiency);
    _inPixelToT.push_back(inToTMap);

    //-----------------------------------------------------------
    name.str(""); title.str("");
    name<< sensor->getName()
         << "InPixelTimingCnt" << _nameSuffix;
    title << sensor->getName()
          << " [InPixel Timing Map counting]";
    TH2D *count = new TH2D(name.str().c_str(), title.str().c_str(),
                           pixBinsX, 0, sensor->getPitchX(),
                           pixBinsY, 0, sensor->getPitchY());
    count->SetDirectory(0);
    _inPixelCounting.push_back(count);





    name.str(""); title.str("");
    name<< sensor->getName()
         << "InPixelTimingCnt1" << _nameSuffix;
    title << sensor->getName()
          << " [InPixel Timing Map counting1]";
    TH2D *count_intiming = new TH2D(name.str().c_str(), title.str().c_str(),
                           pixBinsX, 0, sensor->getPitchX(),
                           pixBinsY, 0, sensor->getPitchY());
    count->SetDirectory(0);
    _inPixelCountingtiming.push_back(count_intiming);


    // Track matching information
    name.str(""); title.str("");
    name<< sensor->getName()
         << "MatchedTracks" << _nameSuffix;
    title << sensor->getName()
          << " [Matched Tracks]";
    TH1D *matched = new TH1D(name.str().c_str(), title.str().c_str(),
                             2, -0.5, 1.5);
    matched->GetXaxis()->SetTitle("0 : unmatched tracks | 1 : matched tracks");
    matched->SetDirectory(plotDir_Efficiency);
    _matchedTracks.push_back(matched);

    //-----------------------------------------------------------
    name.str(""); title.str("");
    name<< sensor->getName()
         <<  "InPixelEfficiency" << _nameSuffix;
    title << sensor->getName()
          << " In Pixel Efficiency"
          << ";X position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Y position [" << _dutDevice->getSpaceUnit() << "]"
          << "";
    TEfficiency *inPixelEfficiency = new TEfficiency(name.str().c_str(), title.str().c_str(),
        pixBinsX, 0, sensor->getPitchX(),
        pixBinsY, 0, sensor->getPitchY());
    inPixelEfficiency->SetDirectory(plotDir_Efficiency);
    _inPixelEfficiency.push_back(inPixelEfficiency);

    //-----------------------------------------------------------
    if (_refDevice->getTimeEnd() > _refDevice->getTimeStart()) // If not used, they are both == 0
    {
      // Prevent aliasing
      const unsigned int nTimeBins = 100;
      const ULong64_t timeSpan = _refDevice->getTimeEnd() - _refDevice->getTimeStart() + 1;
      const ULong64_t startTime = _refDevice->getTimeStart();
      const ULong64_t endTime = timeSpan - (timeSpan % nTimeBins) + startTime;

      name.str(""); title.str("");
      name<< sensor->getName()
           << "EfficiencyVsTime" << _nameSuffix;
      title << sensor->getName()
            << " Efficiency Vs. Time"
            << ";Time [" << _refDevice->getTimeUnit() << "]"
            << ";Average efficiency";
      TEfficiency *efficiencyTime = new TEfficiency(name.str().c_str(), title.str().c_str(),
          nTimeBins,
          _refDevice->tsToTime(startTime),
          _refDevice->tsToTime(endTime + 1));
      efficiencyTime->SetDirectory(plotDir_Efficiency);
      efficiencyTime->SetStatisticOption(TEfficiency::kFWilson);
      _efficiencyTime.push_back(efficiencyTime);
    }
  }
}

//=========================================================
void Analyzers::Efficiency::processEvent(const Storage::Event *refEvent,
    const Storage::Event *dutEvent) {

  assert(refEvent && dutEvent && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(refEvent, dutEvent);

  // Check if the event passes the cuts
  for (unsigned int ncut = 0; ncut < _numEventCuts; ncut++)
    if (!_eventCuts.at(ncut)->check(refEvent)) return;

  for (unsigned int ntrack = 0; ntrack < refEvent->getNumTracks(); ntrack++) {
    const Storage::Track *track = refEvent->getTrack(ntrack);

    // Check if the track passes the cuts
    bool pass = true;
    for (unsigned int ncut = 0; ncut < _numTrackCuts; ncut++)
      if (!_trackCuts.at(ncut)->check(track)) {
        pass = false;
        break;
      }
    if (!pass) continue;

    // Make a list of the planes with their matched cluster
    std::vector<Storage::Cluster *> matches;
    for (unsigned int nplane = 0; nplane < dutEvent->getNumPlanes(); nplane++)
      matches.push_back(0); // No matches

    // Get the matches from the track
    for (unsigned int nmatch = 0; nmatch < track->getNumMatchedClusters(); nmatch++) {
      Storage::Cluster *cluster = track->getMatchedCluster(nmatch);

      // Check if this cluster passes the cuts
      //bool pass = true; // <--------- [SGS] WHY COMMENTED ????
      for (unsigned int ncut = 0; ncut < _numClusterCuts; ncut++) {
        if (!_clusterCuts.at(ncut)->check(cluster)) {
          pass = false;
          break;
        }

        //Bilbao@cern.ch:Cut for cluster size (also in DUTresiduals to affect all eff.root results)
        /*if(cluster->getNumHits()==1){
        std::cout << cluster->getNumHits() << std::endl;
            pass = false;
            break;}*/

        pass = true;
        matches.at(cluster->getPlane()->planeNumber()) = cluster;
      }
    }

    assert(matches.size() == _dutDevice->getNumSensors() &&
           _relativeToSensor < (int)_dutDevice->getNumSensors() &&
           "Efficiency: matches has the wrong size");

    // If asking for relative efficiency to a plane, look for a match in that one first
    if (_relativeToSensor >= 0 && !matches.at(_relativeToSensor)) continue;

    for (unsigned int nsensor = 0; nsensor < _dutDevice->getNumSensors(); nsensor++) {
      const Mechanics::Sensor *sensor = _dutDevice->getSensor(nsensor);

      double tx = 0, ty = 0, tz = 0;
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);

      double px = 0, py = 0;
      sensor->spaceToPixel(tx, ty, tz, px, py);




      const Storage::Cluster *match = matches.at(nsensor);


  if(match){
       _ResX.at(nsensor)->Fill(tx-match->getPosX());
      // _ResY.at(nsensor)->Fill(ty-match->getPosY());
       _ResY.at(nsensor)->Fill(ty-match->getPosY());
       }
  if(!match){
       _ResX.at(nsensor)->Fill(tx);
       _ResY.at(nsensor)->Fill(ty);
       }





      // Check if the intercepted pixel is hit
      //      bool trackMatchesPixel = false;
      //      unsigned int pixelX = px;
      //      unsigned int pixelY = py;
      //      if (match)
      //      {
      //        for (unsigned int nhit = 0; nhit < match->getNumHits(); nhit++)
      //        {
      //          const Storage::Hit* hit = match->getHit(nhit);
      //          if (hit->getPixX() == pixelX && hit->getPixY() == pixelY)
      //          {
      //            trackMatchesPixel = true;
      //            break;
      //          }
      //        }
      //      }

      // Get the location within the pixel where the track interception
      const double trackPosX = px - (int)px;
      const double trackPosY = py - (int)py;






      // TO BE PROPERLY SET!!!!!
      //if (px > 1 & px<6 & py>86 & py <94) {
      //if (px > 1 && px < 5 && py > 82 && py < 84) { //402
      //if (px > 1 && px < 6 && py > 81 && py < 84) { //404 STIME
      //if (px > 1 && px < 6 && py > 86 && py < 95) { //CAribou01
      if ((px > 4 && px < 10) && (py>87 && py < 94)) { //CAribou02 batch4

        //  if(px>1 && px<5  && py>85 && py<87   ){
        //if ( (px > _pix_x_min) && (px < _pix_x_max) && (py > _pix_y_min) && (py < _pix_y_max) ) {
        /*  if(match){
        std::cout << "trackPosX is: " << trackPosX << " AND trackPosY is: " << trackPosY << std::endl;
        std::cout << "pitchX is: " << sensor->getPitchX() << "[pixels] AND pitchY is: " << sensor->getPitchY() << std::endl;
        std::cout << "positionX is: " << sensor->getPitchX()*px << "[] AND positionY is: " << sensor->getPitchY()*py<< std::endl;
        std::cout << " "<< std::endl;}*/
      



          _inPixelEfficiency.at(nsensor)->Fill((bool)match,
                                             trackPosX * sensor->getPitchX(),
                                             trackPosY * sensor->getPitchY());

        _inPixelEfficiencyLVL1_total.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
            trackPosY * sensor->getPitchY());


        (_cluster_total.at(nsensor))[0]->Fill(trackPosX * sensor->getPitchX(),
                                              trackPosY * sensor->getPitchY());



	_inPixelCounting.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
                                               trackPosY * sensor->getPitchY());


        if (match) {
//*
//
//
//
//
//
	_inPixelCountingtiming.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
                                               trackPosY * sensor->getPitchY());

   int clustersize = match->getNumHits()-1;
            if (clustersize > _maxclustersize) {
              clustersize = _maxclustersize - 1;
            }

            (_cluster_passed.at(nsensor))[clustersize]->Fill(trackPosX * sensor->getPitchX(),
                trackPosY * sensor->getPitchY());


           // _inPixelCounting.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
             //                                  trackPosY * sensor->getPitchY());


          float fast_match=0;
          for (unsigned int nhit = 0; nhit < match->getNumHits(); nhit++) {
          const Storage::Hit *hit = match->getHit(nhit);
          if(fast_match==0) fast_match=match->getTiming();
          if((match->getTiming()) &&  (match->getTiming()!=0) ) fast_match=match->getTiming();  //take the fastest time
           }
 

      
 

            _inPixelTiming.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
                                             trackPosY * sensor->getPitchY(),
                                             fast_match);





        double Tfast=0;
        double Tslow=0;
        double max=0;
        const unsigned int Dim=match->getNumHits();
        double timing[Dim]; //*/
        const Storage::Cluster *cluster0;
        const Storage::Hit *hit1;

        int fflag=0;
        int fl=0,fl1=0;
	const unsigned int npoints = track->getNumClusters();
    //   if(npoints>6)continue;

          for(unsigned int k=0;k<npoints;k++){ //loop over the plane
          const Storage::Cluster *cluster = track->getCluster(k);
          if(cluster->getNumHits()!=1)fflag=100;

          
          hit1 = cluster->getHit(0);
          for(unsigned int nhit = 0; nhit < cluster->getNumHits(); nhit++){
         const Storage::Hit *hit0 = cluster->getHit(nhit);
         if(cluster->getNumHits()==1 && npoints==6){
          fl=0;
          if(cluster->getNumHits()==1 && hit0->getTiming()==1)fl1=fl1+1;
          for(unsigned int j=0;j<npoints;j++){ //loop over the plane
          const Storage::Cluster *cluster3 = track->getCluster(j);
          if(cluster3->getNumHits()==1)fl=fl+1;
           //Want all the plane to have CL=1
         }
         if(fl==6){_TimingPlot.at(nsensor)[k]->Fill(hit0->getTiming());
         if(match->getNumHits()==1){ const Storage::Hit *hitmatch = match->getHit(0);
           _TimingCorr.at(nsensor)[k]->Fill(hitmatch->getTiming(), hit0->getTiming());}}

         if(k<5) {
         const Storage::Cluster *clusternext = track->getCluster(k+1);
         for(unsigned int nhitnext = 0; nhitnext < clusternext->getNumHits(); nhitnext++){
         const Storage::Hit *hitnext = clusternext->getHit(nhitnext);
         if(clusternext->getNumHits()==1){
        //  if(fl==6)_TimingCorr.at(nsensor)[k]->Fill(hit0->getTiming(), hitnext->getTiming());

            }
           }
          }
         }//close if
        } //close loop per track

          if(hit1->getTiming()==1)fflag=fflag+1;  //asking for get plane
       //   cout<<cluster->getPlane()->getPlaneNum()<<endl;
          if(cluster->getPlane()->planeNumber()==2) cluster0 = track->getCluster(k);
          }


        const unsigned int Dim0=cluster0->getNumHits(); //fastest_hit_plane0
        double timing0=0;
          for (unsigned int nhit = 0; nhit < cluster0->getNumHits(); nhit++) {
          const Storage::Hit *hit0 = cluster0->getHit(nhit);
          if(timing0==0) timing0=hit0->getTiming();
          if((timing0>hit0->getTiming()) &&  (hit0->getTiming()!=0) ) timing0=hit0->getTiming();  //take the fastest time
           }
         

        //  if(fl!=6 || fl1!=6)continue; //want to select single hit in all plane


//*
          for (unsigned int nhit = 0; nhit < match->getNumHits(); nhit++) {
            const Storage::Hit *hit = match->getHit(nhit);
            

            _matchTime.at(nsensor)->Fill(hit->getTiming());
      if(nhit==0){
          for (unsigned int nhit1 = 0; nhit1 < match->getNumHits(); nhit1++) {
            const Storage::Hit *hit1 = match->getHit(nhit1);
            timing[nhit1]=(hit1->getTiming());
             }
          if(nhit==0) max=timing[0];
          if(match->getNumHits()==1) max=timing[0];
          else
          for(unsigned int y = 0; y < match->getNumHits(); y++){
           if(max>timing[y+1]) max=timing[y+1];
            }
           }
           
         //  std::sort(time.begin(), time.end());
          


            //if (nhit!=0){
/*
            _inPixelTiming.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
                                             trackPosY * sensor->getPitchY(),
                                             hit->getTiming());
*/
            _inPixelToT.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
                                          trackPosY * sensor->getPitchY(),
                                          hit->getValue());


   
          //-----lvl1 correlation plot
// /

            int lvl1 = (int)hit->getTiming();
            (_triggerPhaseDUT.at(nsensor))[lvl1]->Fill(dutEvent->getTriggerPhase());



            (_inPixelEfficiencyLVL1_passed.at(nsensor))[lvl1]->Fill(trackPosX * sensor->getPitchX(),
                trackPosY * sensor->getPitchY());

            (_inPixelTimingLVL1.at(nsensor))[lvl1]->Fill(trackPosX * sensor->getPitchX(),
                trackPosY * sensor->getPitchY());



            const unsigned int npoints = track->getNumClusters();
            for (unsigned int k = 0; k < npoints; k++) { //loop over the plane
              const Storage::Cluster *cluster_TDC_ref = track->getCluster(k);
              cluster_TDC_ref->getPlane();
              int lvl1_ref = cluster_TDC_ref->getTiming();
              //cout<<k<<" "<<cluster_TDC_ref->getTiming()<<endl;
              (_triggerPhaseREF.at(k))[lvl1_ref]->Fill(refEvent->getTriggerPhase());
            //  _LVL1TriggerREF.at(k)->Fill(lvl1_ref);
            }

            /*
                     const unsigned int npoints = track->getNumClusters();
                     const unsigned int npoints = track->getNumClusters();
                //   if(npoints>6)continue;



            if(nhit==0) Tfast=hit->getTiming();
            if(nhit>0){




            */

            if (nhit == 0) Tfast = hit->getTiming();
            if (nhit > 0) {
              if (Tfast > hit->getTiming()) Tfast = hit->getTiming(); //only the fastest is kept
            }
            if (nhit == 0) Tslow = hit->getTiming();
            if (nhit > 0) {
              if (Tslow < hit->getTiming()) Tslow = hit->getTiming(); //only the fastest is kept
            }

//*/


            int clustersize = match->getNumHits()-1;
            if (clustersize > _maxclustersize) {
              clustersize = _maxclustersize - 1;
            }
	    

           double ttime=timing[nhit];

	   (_TimingCluster.at(nsensor))[clustersize]->Fill(hit->getTiming());
            
            if(nhit==match->getNumHits()-1) (_TimingCluster_fast.at(nsensor))[clustersize]->Fill(std::abs(Tfast));
            if(nhit==match->getNumHits()-1) (_timing_diff.at(nsensor))->Fill(std::abs(Tfast-timing0));
            if( nhit>0) {

            if(max<hit->getTiming()) (_TimingCluster_slow.at(nsensor))[clustersize]->Fill(ttime);

            }
            if(match->getNumHits()==1) (_TimingCluster_slow.at(nsensor))[clustersize]->Fill(ttime);
           
            




            //(_cluster_passed.at(nsensor))[clustersize]->Fill(trackPosX * sensor->getPitchX(),
             //   trackPosY * sensor->getPitchY());
          
  
            (_TimingTOT_cluster.at(nsensor))[clustersize]->Fill(hit->getTiming(), hit->getValue());
           }
          
        }
      }



      if (match)
      {

        //_efficiencyMap.at(nsensor)->Fill(true, tx, ty);
/*
	  double Tfast=0;
	  double Tslow=0;
	  double max=0;
	  const unsigned int Dim=match->getNumHits();
	  double timing[Dim];
 */
        /*
            double Tfast=0;
            double Tslow=0;
            double max=0;
            const unsigned int Dim=match->getNumHits();
            double timing[Dim];
         */
        for (unsigned int nhit = 0; nhit < match->getNumHits(); nhit++) {
          const Storage::Hit *hit = match->getHit(nhit);
//          _matchTime.at(nsensor)->Fill(hit->getTiming());
          _matchToT.at(nsensor)->Fill(hit->getValue());
          _lvl1vsToT.at(nsensor)->Fill(hit->getTiming(), hit->getValue());

          
   /*       if(nhit==0){
          for (unsigned int nhit1 = 0; nhit1 < match->getNumHits(); nhit1++) {
            const Storage::Hit *hit1 = match->getHit(nhit1);
            timing[nhit1]=(hit1->getTiming());
             }
          if(nhit==0) max=timing[0];
          if(match->getNumHits()==1) max=timing[0];
          else
          for(unsigned int y = 0; y < match->getNumHits(); y++){
           if(max>timing[y+1]) max=timing[y+1];
            }
           }

            if(nhit==0) Tfast=hit->getTiming();
            if(nhit>0){
             if(Tfast>hit->getTiming()) Tfast=hit->getTiming();  //only the fastest is kept
            }
            if(nhit==0) Tslow=hit->getTiming();
            if(nhit>0){
             if(Tslow<hit->getTiming()) Tslow=hit->getTiming();  //only the fastest is kept
            }


            int clustersize = nhit match->getNumHits()-1;
            if (clustersize > _maxclustersize) {
              clustersize = _maxclustersize-1;
            }

           double ttime=timing[nhit];

	   (_TimingCluster.at(nsensor))[clustersize]->Fill(hit->getTiming());
            if(nhit==match->getNumHits()-1) (_TimingCluster_fast.at(nsensor))[clustersize]->Fill(Tfast);
            if( nhit>0) {

            if(max<hit->getTiming()) (_TimingCluster_slow.at(nsensor))[clustersize]->Fill(ttime);

            }
            if(match->getNumHits()==1) (_TimingCluster_slow.at(nsensor))[clustersize]->Fill(ttime);
   */


          /*       if(nhit==0){
                 for (unsigned int nhit1 = 0; nhit1 < match->getNumHits(); nhit1++) {
                   const Storage::Hit *hit1 = match->getHit(nhit1);
                   timing[nhit1]=(hit1->getTiming());
                    }
                 if(nhit==0) max=timing[0];
                 if(match->getNumHits()==1) max=timing[0];
                 else
                 for(unsigned int y = 0; y < match->getNumHits(); y++){
                  if(max>timing[y+1]) max=timing[y+1];
                   }
                  }

                   if(nhit==0) Tfast=hit->getTiming();
                   if(nhit>0){
                    if(Tfast>hit->getTiming()) Tfast=hit->getTiming();  //only the fastest is kept
                   }
                   if(nhit==0) Tslow=hit->getTiming();
                   if(nhit>0){
                    if(Tslow<hit->getTiming()) Tslow=hit->getTiming();  //only the fastest is kept
                   }


                   int clustersize = nhit match->getNumHits()-1;
                   if (clustersize > _maxclustersize) {
                     clustersize = _maxclustersize-1;
                   }

                  double ttime=timing[nhit];

            (_TimingCluster.at(nsensor))[clustersize]->Fill(hit->getTiming());
                   if(nhit==match->getNumHits()-1) (_TimingCluster_fast.at(nsensor))[clustersize]->Fill(Tfast);
                   if( nhit>0) {

                   if(max<hit->getTiming()) (_TimingCluster_slow.at(nsensor))[clustersize]->Fill(ttime);

                   }
                   if(match->getNumHits()==1) (_TimingCluster_slow.at(nsensor))[clustersize]->Fill(ttime);
          */
        }





        //bilbao@cern.ch / reina.camacho@cern.ch: Filling the efficiency histogram with the cluster position instead of the track position


        _matchCluster.at(nsensor)->Fill(match->getNumHits());
    
          int flag=0;
          for (unsigned int nhit = 0; nhit < match->getNumHits(); nhit++) {
          const Storage::Hit *hit = match->getHit(nhit);
          if(hit->getTiming()<4) flag=1;
         }
        //fare un efficienza per il fastest hit, dare un taglio sul fastest hit: cioè un reclustering al vol. fai copia e incolla dal clustermaker.
       /* if(flag==1)*/_efficiencyMap.at(nsensor)->Fill(true, match->getPosX(), match->getPosY());
       /* if(flag==0)_efficiencyMap.at(nsensor)->Fill(false, tx, ty);*/
        _matchedTracks.at(nsensor)->Fill(1);
        _matchPositionX.at(nsensor)->Fill(std::sqrt(std::pow((tx - match->getPosX()) / 100., 2) + std::pow((ty - match->getPosY()) / 125., 2)));
        _matchPositionY.at(nsensor)->Fill(ty);


      //  if (_efficiencyTime.size())
       //   _efficiencyTime.at(nsensor)->Fill(true, _refDevice->tsToTime(refEvent->getTimeStamp()));










        if (_efficiencyTime.size())
          _efficiencyTime.at(nsensor)->Fill(true, _refDevice->tsToTime(refEvent->getTimeStamp()));
      }
      else
      {
        _efficiencyMap.at(nsensor)->Fill(false, tx, ty);
        _matchedTracks.at(nsensor)->Fill(0);
        if (_efficiencyTime.size())
          _efficiencyTime.at(nsensor)->Fill(false, _refDevice->tsToTime(refEvent->getTimeStamp()));
      }

    }
  }
}



//=========================================================
void Analyzers::Efficiency::postProcessing() {
  if (_postProcessed) return;

  for (unsigned int nsensor = 0; nsensor < _dutDevice->getNumSensors(); nsensor++) {

    TEfficiency *efficiency = _efficiencyMap.at(nsensor);
    const TH1 *values = efficiency->GetTotalHistogram();
    TH1D *distribution = _efficiencyDistribution.at(nsensor);

    // InPixel timing/ToT plots and renormalization
    TH2D *inTimingMap =  _inPixelTiming.at(nsensor);
    TH2D *inToTMap =  _inPixelToT.at(nsensor);
    TH2D *count = _inPixelCounting.at(nsensor);
    TH2D *count_intiming = _inPixelCountingtiming.at(nsensor);

    double val = 0;
    for (int x = 1; x <= inTimingMap->GetNbinsX(); x++) {
      for (int y = 1; y <= inTimingMap->GetNbinsY(); y++) {

        val = count->GetBinContent(x, y) != 0 ? inTimingMap->GetBinContent(x, y) / count_intiming->GetBinContent(x, y) : 0;
        // [SGS] why overwritting the original histo instead of filling a new one ???
        inTimingMap->SetBinContent(x, y, val);

        val = count->GetBinContent(x, y) != 0 ? inToTMap->GetBinContent(x, y) / count_intiming->GetBinContent(x, y) : 0;
        // [SGS] why overwritting the original histo instead of filling a new one ???
        inToTMap->SetBinContent(x, y, val);

        // loop in the 16 LVL1 bunch-crossings
        for (int bc = 0; bc < 16; bc++) {
          TH2D *hist = _inPixelTimingLVL1.at(nsensor)[bc];

          if ( hist->GetBinContent(x, y) > _min_entries_lvl1 ) {
            val = count->GetBinContent(x, y) != 0 ? hist->GetBinContent(x, y) / count->GetBinContent(x, y) : 0;
            // [SGS] why overwritting the original histo instead of filling a new one ???
            hist->SetBinContent(x, y, val);
          }

          // for matched tracks
          TH2D *hpass = _inPixelEfficiencyLVL1_passed.at(nsensor)[bc];
          TH2D *htot = _inPixelEfficiencyLVL1_total[nsensor];
          //cout << "Min min_entries_lvl1_matchedTracks " << _min_entries_lvl1_matchedTracks << endl;
          if ( hpass->GetBinContent(x, y) > _min_entries_lvl1_matchedTracks ) {
            val = htot->GetBinContent(x, y) != 0 ?  hpass->GetBinContent(x, y) / htot->GetBinContent(x, y) : 0;
            // [SGS] why overwritting the original histo instead of filling a new one ???
            hpass->SetBinContent(x, y, val);
          }
        } // end loop in BC

        for (int cl = 0; cl <= _maxclustersize; ++cl)
        {
          TH2D *hpass_cl = _cluster_passed.at(nsensor)[cl];
          TH2D *htot_cl  = _cluster_total.at(nsensor)[0];
          if ( hpass_cl->GetBinContent(x, y) > _min_entries_lvl1_matchedTracks ) {
            val = htot_cl->GetBinContent(x, y) != 0 ?  hpass_cl->GetBinContent(x, y) / htot_cl->GetBinContent(x, y) : 0;
            // [SGS] why overwritting the original histo instead of filling a new one ???
            hpass_cl->SetBinContent(x, y, val);
          } //end loop in cluster size
        }
      }
    }

    // Loop over all pixel groups
    for (int binx = 1; binx <= values->GetNbinsX(); ++binx) {
      for (int biny = 1; biny <= values->GetNbinsY(); ++biny) {
        const Int_t bin = values->GetBin(binx, biny);
        if (values->GetBinContent(binx, biny) < 1) continue;
        const double value = efficiency->GetEfficiency(bin);
        const double sigmaLow = efficiency->GetEfficiencyErrorLow(bin);
        const double sigmaHigh = efficiency->GetEfficiencyErrorUp(bin);
        // Find the probability of this pixel group being found in all bins of the distribution
        double normalization = 0;

        for (int distBin = 1; distBin <= distribution->GetNbinsX(); distBin++) {
          const double evaluate = distribution->GetBinCenter(distBin);
          const double sigma = (evaluate < value) ? sigmaLow : sigmaHigh;
          const double weight = TMath::Gaus(evaluate, value, sigma);
          normalization += weight;
        }

        for (int distBin = 1; distBin <= distribution->GetNbinsX(); distBin++) {
          const double evaluate = distribution->GetBinCenter(distBin);
          const double sigma = (evaluate < value) ? sigmaLow : sigmaHigh;
          const double weight = TMath::Gaus(evaluate, value, sigma);
          distribution->Fill(evaluate, weight / normalization);
        }
      }
    }
  }
  _postProcessed = true;
}
