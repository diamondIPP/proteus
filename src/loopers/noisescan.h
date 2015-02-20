#ifndef NOISESCAN_H
#define NOISESCAN_H

#include "looper.h"

namespace Storage { class StorageIO; }
namespace Mechanics { class Device; }
namespace Processors { class ClusterMaker; }
namespace Processors { class TrackMaker; }

namespace Loopers {

  /** @class NoiseScanConfig
   **    
   ** @brief A class to contain configuration paramaters for the NoiseScan looper.
   **
   ** This class is also used by the NoiseMask class to read/write
   ** the configuration parameters of the NoiseScan looper used to create
   ** the mask file, and propagated to the output summary TTree.
   */
  class NoiseScanConfig {
  public:
    NoiseScanConfig();
    NoiseScanConfig(const std::vector<int>& runs);
    NoiseScanConfig(const NoiseScanConfig&);
    NoiseScanConfig &operator=(const NoiseScanConfig&);

    void setRuns(std::vector<int>& runs) { _vruns = runs; }
    void setMaxFactor(double maxFactor) { _maxFactor = maxFactor; }
    void setMaxOccupancy(double maxOccupancy) { _maxOccupancy = maxOccupancy; }
    void setBottomLimitX(int bottomLimitX) { _bottomLimitX = bottomLimitX; }
    void setUpperLimitX(int upperLimitX) { _upperLimitX = upperLimitX; }
    void setBottomLimitY(int bottomLimitY){ _bottomLimitY = bottomLimitY; }
    void setUpperLimitY(int upperLimitY) { _upperLimitY = upperLimitY; }

    std::vector<int> getRuns() const { return _vruns; }
    double getMaxFactor() const { return _maxFactor; }
    double getMaxOccupancy() const { return _maxOccupancy; }
    int getBottomLimitX() const { return _bottomLimitX; }
    int getUpperLimitX() const { return _upperLimitX; }
    int getBottomLimitY() const { return _bottomLimitY; }
    int getUpperLimitY() const  { return _upperLimitY; }

    const std::string print() const;
    
  private:
    double _maxFactor;
    double _maxOccupancy;
    int _bottomLimitX;
    int _upperLimitX;
    int _bottomLimitY;
    int _upperLimitY;
    
    std::vector<int> _vruns;
  };
  
  /** @class NoiseScan
   **    
   ** @brief Looper derived class used to find noisy pixels based on occupancy analysis.
   */
  class NoiseScan : public Looper {
  public:
    NoiseScan(Mechanics::Device* refDevice,
	      Storage::StorageIO* refInput,
	      ULong64_t startEvent=0,
	      ULong64_t numEvents=0,
	      unsigned int eventSkip=1);
    ~NoiseScan();
    
    /** Loops in events, fills occupancy distributions, determines noisy pixels 
	for all planes according to configuration parameters set in NoiseScanConfig.
	The output mask file is also created here. */
    void loop();
    
    void print() const; 
    
    void setMaxFactor(double maxFactor) { _config->setMaxFactor(maxFactor); }
    void setMaxOccupancy(double maxOccupancy) { _config->setMaxOccupancy(maxOccupancy); }
    void setBottomLimitX(int bottomLimitX) { _config->setBottomLimitX(bottomLimitX); }
    void setUpperLimitX(int upperLimitX) { _config->setUpperLimitX(upperLimitX); }
    void setBottomLimitY(int bottomLimitY) { _config->setBottomLimitY(bottomLimitY); }
    void setUpperLimitY(int upperLimitY) { _config->setUpperLimitY(upperLimitY); }
    void setPrintLevel(int printLevel) { _printLevel = printLevel; }

  private:
    Mechanics::Device* _refDevice;
    NoiseScanConfig* _config;
    int _printLevel;    
  }; 
  
} // end of namespace 

#endif // NOISESCAN_H
