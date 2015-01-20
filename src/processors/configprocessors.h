#ifndef CONFIGPROCESSORS_H
#define CONFIGPROCESSORS_H

class ConfigParser;

namespace Processors {
  
  class TrackMaker;
  class TrackMatcher;
  class ClusterMaker;
  
  ClusterMaker* generateClusterMaker(const ConfigParser& config);
  
  TrackMaker* generateTrackMaker(const ConfigParser& config, bool align = false);

} // end of namespace

#endif // CONFIGURE_H
