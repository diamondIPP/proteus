#ifndef CONFIGMECHANICS_H
#define CONFIGMECHANICS_H

#include <vector>

class ConfigParser;

namespace Mechanics {
  
  class Device;
  class Sensor;
  
  Device* generateDevice(const ConfigParser& config);

  void generateSensors(const ConfigParser& config, Device* device);  

} // end of namespace

#endif // CONFIGMECHANICS_H
