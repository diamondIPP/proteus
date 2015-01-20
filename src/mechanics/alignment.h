#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <string>

namespace Mechanics {

  class Device;
  
  class Alignment{
  public:
    Alignment(const char* fileName, Device* device);
    
    const char* getFileName();
    
    void readFile();
    void writeFile();
    
  private:
    unsigned int sensorFromHeader(std::string header);	
    
  private:
    std::string _fileName;
    Device* _device;  
    
  }; // end of class 
  
} // end of namespace 

#endif // ALIGNMENT_H
