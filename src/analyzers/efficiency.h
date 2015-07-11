#ifndef EFFICIENCY_H
#define EFFICIENCY_H

#include <vector>

#include <TH2D.h>
#include <TH1D.h>
#include <TEfficiency.h>
#include <TDirectory.h>

#include "dualanalyzer.h"

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

class Efficiency : public DualAnalyzer
{
public:
    Efficiency(const Mechanics::Device *refDevice,
               const Mechanics::Device *dutDevice,
               TDirectory *dir = 0,
               const char *suffix = "",
               int relativeToSensor = -1, // Only consider events where this sensor has a match
               int pix_x_min = 1,
               int pix_x_max = 12,
               int pix_y_min = 81,
               int pix_y_max = 94,
               int min_entries_lvl1 = 51,
               int min_entries_lvl1_matchedTracks = 0,
               /* Histogram options */
               unsigned int rebinX = 1, // This many pixels in X are grouped
               unsigned int rebinY = 1,
               unsigned int pixBinsX = 20,
               unsigned int pixBinsY = 20);
    int _maxclustersize; //Max Dimension of cluster size
    void processEvent(const Storage::Event *refEvent,
                      const Storage::Event *dutDevent);

    void postProcessing();



private:
    int _relativeToSensor;
    int _pix_x_min; //!< minimum pixel (x) to be considered for in-pixel results
    int _pix_x_max; //!< maximum pixel (x) to be considered for in-pixel results
    int _pix_y_min; //!< minimum pixel (y) to be considered for in-pixel results
    int _pix_y_max; //!< maximum pixel (y) to be considered for in-pixel resultsa

    int _min_entries_lvl1; //!< minimum number of entries in LVL1-vs-BC timing map
    int _min_entries_lvl1_matchedTracks;  //!< minimum number of entries in LVL1-vs-BC timing map (matched tracks)


    std::vector<TEfficiency *> _efficiencyMap;
    std::vector<TH1D *> _efficiencyDistribution; // name to be changed

    std::vector<TH1D *> _matchedTracks;
    std::vector<TH1D *> _matchPositionX;
    std::vector<TH1D *> _matchPositionY;
    std::vector<TH1D *> _matchTime;
    std::vector<TH1D *> _matchCluster;
    std::vector<TH1D *> _matchToT;

    std::vector<TEfficiency *> _efficiencyTime;
    std::vector<TEfficiency *> _inPixelEfficiency;
    std::vector<TH2D *> _inPixelTiming;
    std::vector<TH2D *> _inPixelCounting;
    std::vector<TH2D *> _inPixelToT;
    std::vector<TH2D *> _lvl1vsToT;
    
    typedef std::vector<TH1D *> vecTH1D;
    typedef std::vector<TH2D *> vecTH2D;

    std::vector<vecTH1D> _TimingCluster;

    std::vector<vecTH2D> _inPixelTimingLVL1; // name to be changed
    std::vector<vecTH2D> _TimingTOT_cluster; // name to be changed
    
    std::vector<TH2D *>  _inPixelEfficiencyLVL1_total; // name to be changed
    std::vector<vecTH2D> _inPixelEfficiencyLVL1; // name to be changed
    std::vector<vecTH2D> _inPixelEfficiencyLVL1_passed; // name to be changed

    
    std::vector<vecTH2D> _cluster_total; 
    std::vector<vecTH2D> _cluster;
    std::vector<vecTH2D> _cluster_passed;

}; // end of class

} // end of namespace

#endif // EFFICIENCY_H
