#ifndef CONFIGANALYZERS_H
#define CONFIGANALYZERS_H

#include <TFile.h>

#include "utils/configparser.h"

namespace Mechanics { class Device; }
namespace Loopers { class Looper; }

namespace Analyzers {
  
  class HitCut;
  class ClusterCut;
  class TrackCut;
  class EventCut;
  class BaseAnalyzer;
  class SingleAnalyzer;
  class DualAnalyzer;
  class Correlation;
  class DutCorrelation;
  class Efficiency;
  class HitInfo;
  class Matching;
  class Occupancy;
  class DUTResiduals;
  class TrackInfo;

  void parseCut(const ConfigParser::Row* row,
		std::vector<EventCut*>& eventCuts,
		std::vector<TrackCut*>& trackCuts,
		std::vector<ClusterCut*>& clusterCuts,
		std::vector<HitCut*>& hitCuts);
  
  void applyCuts(BaseAnalyzer* analyzer,
		 std::vector<EventCut*>& eventCuts,
		 std::vector<TrackCut*>& trackCuts,
		 std::vector<ClusterCut*>& clusterCuts,
		 std::vector<HitCut*>& hitCuts);
  
  void configLooper(const ConfigParser& config,
		    Loopers::Looper* looper,
		    Mechanics::Device* refDevice,
		    Mechanics::Device* dutDevice,
		    TFile* results);
  
  void configDepictor(const ConfigParser& config,
		      Loopers::Looper* looper,
		      Mechanics::Device* refDevice);
  
  void configDUTDepictor(const ConfigParser& config,
			 Loopers::Looper* looper,
			 Mechanics::Device* refDevice,
			 Mechanics::Device* dutDevice);
  
  void configCorrelation(const ConfigParser& config,
			 Loopers::Looper* looper,
			 Mechanics::Device* refDevice,
			 TFile* results);
  
  void configDUTCorrelation(const ConfigParser& config,
			    Loopers::Looper* looper,
			    Mechanics::Device* refDevice,
			    Mechanics::Device* dutDevice,
			    TFile* results);
  
  void configHitInfo(const ConfigParser& config,
		     Loopers::Looper* looper,
		     Mechanics::Device* refDevice,
		     TFile* results);
  
  void configClusterInfo(const ConfigParser& config,
			 Loopers::Looper* looper,
			 Mechanics::Device* refDevice,
			 TFile* results);
  
  void configOccupancy(const ConfigParser& config,
		       Loopers::Looper* looper,
		       Mechanics::Device* refDevice,
		       TFile* results);
  
  void configTrackInfo(const ConfigParser& config,
		       Loopers::Looper* looper,
		       Mechanics::Device* refDevice,
		       TFile* results);
  
  void configEventInfo(const ConfigParser& config,
		       Loopers::Looper* looper,
		       Mechanics::Device* refDevice,
		       TFile* results);
  
  void configResiduals(const ConfigParser& config,
		       Loopers::Looper* looper,
		       Mechanics::Device* refDevice,
		       TFile* results);
  
  void configResiduals(const ConfigParser& config,
		       Loopers::Looper* looper,
		       Mechanics::Device* refDevice,
		       Mechanics::Device* dutDevice,
		       TFile* results);
  
  void configMatching(const ConfigParser& config,
		      Loopers::Looper* looper,
		      Mechanics::Device* refDevice,
		      Mechanics::Device* dutDevice,
		      TFile* results);
  
  void configEfficiency(const ConfigParser& config,
			Loopers::Looper* looper,
			Mechanics::Device* refDevice,
			Mechanics::Device* dutDevice,
			TFile* results);
} // end of namespace


#endif // CONFIGANALYZERS_H
