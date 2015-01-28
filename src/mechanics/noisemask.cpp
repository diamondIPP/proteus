#include "noisemask.h"

#include <cassert>
#include <algorithm>
#include <iostream>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "device.h"
#include "sensor.h"
#include "../loopers/noisescan.h"
#include "../configparser.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

using std::fstream;
using std::cout;
using std::endl;
using std::string;

//=========================================================
Mechanics::NoiseMask::NoiseMask(const char* fileName,
				Device* device) :
  _fileName(fileName),
  _device(device),
  _config(0)
{
  assert(device && "NoiseMask can't initialize with a null device");
  _config = new Loopers::NoiseScanConfig();
}

//=========================================================
Mechanics::NoiseMask::~NoiseMask(){
  delete _config;
}

//=========================================================
void Mechanics::NoiseMask::writeMask(const Loopers::NoiseScanConfig *config){
  std::fstream file;
  file.open(_fileName.c_str(), std::ios_base::out);
  
  if (!file.is_open()){
    std::string err("[NoiseMask::writeMask] ");
    err+"unable to open file '"+_fileName+"' for writting";
    throw err;
  }

  // write noisy pixel information
  for (unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
    Sensor* sensor = _device->getSensor(nsens);
    for (unsigned int x=0; x<sensor->getNumX(); x++)
      for (unsigned int y=0; y<sensor->getNumY(); y++)
	if (sensor->isPixelNoisy(x,y))
	  file << nsens << ", " << x << ", " << y << endl;
  }

  // update config info and write looper metadata
  delete _config;    
  _config = new Loopers::NoiseScanConfig(*config);
  file << _config->print() << endl;

  file.close();
}

//=========================================================
void Mechanics::NoiseMask::readMask() {
  std::fstream file;
  file.open(_fileName.c_str(), std::ios_base::in);
  
  if (!file.is_open()){
    cout << "[NoiseMask::readMask] WARNING :: Noise mask failed to open file '"
	 << _fileName << "'" << endl;
    return;
  }
  cout << "Noise mask file '" << _fileName << "' opened OK" << endl;

  std::stringstream comments;
  while (file.good()){
    unsigned int nsens=0;
    unsigned int x=0;
    unsigned int y=0;
    std::string line;
    std::getline(file, line);
    if(!line.size()) continue;
    if(line[0] == '#') { comments << line << "\n"; continue; }
    std::stringstream lineStream(line);
    parseLine(lineStream, nsens, x, y);
    if(nsens >= _device->getNumSensors())
      throw "NoiseMask: specified sensor outside device range";
    if(x >= _device->getSensor(nsens)->getNumX())
      throw "NoiseMask: specified x position outside sensor range";
    if(y >= _device->getSensor(nsens)->getNumY())
      throw "NoiseMask: specified y position outside sensor range";
    _device->getSensor(nsens)->addNoisyPixel(x,y);
  }

  file.close();

  if( !comments.str().empty() )
    parseComments(comments);
}


//=========================================================
void Mechanics::NoiseMask::parseLine(std::stringstream& line,
				     unsigned int& nsens,
				     unsigned int& x,
				     unsigned int& y) {
  unsigned int counter = 0;
  while(line.good()){
    std::string value;
    std::getline(line, value, ',');
    std::stringstream convert(value);
    switch (counter){
    case 0:
      convert >> nsens;
      break;
    case 1:
      convert >> x;
      break;
    case 2:
      convert >> y;
      break;
    }
    counter++;
  }
  if (counter != 3)
    throw "[NoiseMask::parseLine] bad line found in file";  
}

//=========================================================
void Mechanics::NoiseMask::parseComments(std::stringstream& comments){
  if(VERBOSE){
    cout << "\n[NoiseMask::parseComments] input string:"  << endl;
    cout << comments.str() << endl;
  }

  // remove '#' characters from input string
  std::string str(comments.str());
  str.erase(std::remove(str.begin(),str.end(),'#'),str.end());
  
  // create temporary file to write metadata
  char nameBuff[] = "/tmp/noiseMask.XXXXXX";
  int fd = mkstemp(nameBuff);
  if(fd == -1){
    std::string err("[NoiseMask::parseComments] ERROR can't create temp file");
    err+=string(nameBuff)+"'";
    throw err.c_str();
  }
  if(VERBOSE) cout << "temp file '" << nameBuff << "' created OK" << endl;
  write(fd, str.c_str(), strlen(str.c_str()));
  close(fd);

  // use ConfigParser to parse contents
  ConfigParser parser(nameBuff);
  if(VERBOSE) parser.print();

  for (unsigned int i=0; i<parser.getNumRows(); i++){
    const ConfigParser::Row* row = parser.getRow(i);    
    
    if(row->isHeader) continue;
    if(row->header.compare("Noise Scan")) continue;

    if (!row->key.compare("runs")){
      std::vector<int> runs;
      ConfigParser::valueToVec(row->value, runs);
      _config->setRuns(runs);
      runs.clear();
    }
    else if (!row->key.compare("max factor"))
      _config->setMaxFactor(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("max occupancy"))
      _config->setMaxOccupancy(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("bottom x"))
      _config->setBottomLimitX(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("upper x"))
      _config->setUpperLimitX(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("bottom y"))
      _config->setBottomLimitY(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("upper y"))
      _config->setUpperLimitY(ConfigParser::valueToNumerical(row->value));  
    else{
      cout << "[NoiseMask::parseComments] WARNING can't parse row with key '"
	   << row->key << endl;
    }
  }
  
  // delete temporary file
  unlink(nameBuff);
}


//=========================================================
std::vector<bool**> Mechanics::NoiseMask::getMaskArrays() const {
  std::vector<bool**> masks;
  for(unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++)
    masks.push_back(_device->getSensor(nsens)->getNoiseMask());
  return masks;  
}

