#ifndef CONFIGLOOPERS_H
#define CONFIGLOOPERS_H

class ConfigParser;

namespace Loopers {
  
  class FineAlign;
  class FineAlignDut;
  class CoarseAlign;
  class CoarseAlignDut;
  class NoiseScan;
  class Synchronize;

  void configSynchronize(const ConfigParser& config, Synchronize& sync);
  void configNoiseScan(const ConfigParser& config, NoiseScan& noiseScan);
  void configCoarseAlign(const ConfigParser& config, CoarseAlign& coarseAlign);
  void configCoarseAlign(const ConfigParser& config, CoarseAlignDut& coarseAlign);
  void configFineAlign(const ConfigParser& config, FineAlign& fineAlign);
  void configFineAlign(const ConfigParser& config, FineAlignDut& fineAlign);
  
} // end of namespace

#endif // CONFIGLOOPERS_H
