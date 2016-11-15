#include "configprocessors.h"

#include <cassert>

#include "mechanics/device.h"
#include "utils/configparser.h"
#include "trackmaker.h"
#include "matcher.h"
#include "clustermaker.h"

namespace Processors {

  //=========================================================
  ClusterMaker* generateClusterMaker(const ConfigParser& config) {
    unsigned int maxSeparationX = 0;
    unsigned int maxSeparationY = 0;
    double maxSeparation = 0;
    
    for (unsigned int i=0; i<config.getNumRows(); i++){
      const ConfigParser::Row* row = config.getRow(i);
      
      if (row->isHeader && !row->header.compare("End Clustering")){

	if (maxSeparationX == 0 && maxSeparationY == 0 && maxSeparation == 0)
	  throw "Processors: not enough parameters to produce cluster maker";
	
	return new ClusterMaker(maxSeparationX, maxSeparationY, maxSeparation);
      }
      
      if (row->isHeader)
	continue;
      
      if (row->header.compare("Clustering"))
	continue; // Skip non-device rows
      
      if (!row->key.compare("separation x"))
	maxSeparationX = ConfigParser::valueToNumerical(row->value);
      else if (!row->key.compare("separation y"))
	maxSeparationY = ConfigParser::valueToNumerical(row->value);
      else if (!row->key.compare("separation"))
	maxSeparation = ConfigParser::valueToNumerical(row->value);
      else
	throw "Processors: can't parse cluster maker row";
    }
    throw "Processors: didn't produce a cluster maker";
  }

  //=========================================================
  TrackMaker* generateTrackMaker(const ConfigParser& config, bool align) {
    double maxClusterSep = -1;
    unsigned int numSeedPlanes = 1;
    unsigned int minClusters = 3;
    // bool calcIntercepts = false;
    
    const char* header = align ? "Tracking Align" : "Tracking";
    const char* footer = align ? "End Tracking Align" : "End Tracking";
    
    for (unsigned int i=0; i<config.getNumRows(); i++) {
      const ConfigParser::Row* row = config.getRow(i);
      
      if (row->isHeader && !row->header.compare(footer))
	return new TrackMaker(maxClusterSep, numSeedPlanes, minClusters);
      
      if (row->isHeader)
	continue;
      
      if (row->header.compare(header))
	continue; // Skip non-device rows
      
      if (!row->key.compare("seed planes"))
	numSeedPlanes = ConfigParser::valueToNumerical(row->value);
      else if (!row->key.compare("min hit planes"))
	minClusters = ConfigParser::valueToNumerical(row->value);
      else if (!row->key.compare("max cluster dist"))
	maxClusterSep = ConfigParser::valueToNumerical(row->value);
	//   else if (!row->key.compare("calculate intercepts"))
	// calcIntercepts = ConfigParser::valueToLogical(row->value);
      else
	throw "Processors: can't parse track maker row";
    }
    
    throw "Processors: didn't produce a track maker";
  }
  
} // end of namespace
