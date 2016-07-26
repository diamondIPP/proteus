#include "noisemask.h"

#include <cassert>
#include <algorithm>
#include <iostream>
#include <stdexcept>

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

Mechanics::NoiseMask::NoiseMask(const Loopers::NoiseScanConfig* config)
    : _config(new Loopers::NoiseScanConfig(*config)), _fileName("<transient>")
{
}

//=========================================================
Mechanics::NoiseMask::NoiseMask(const std::string& path)
    : _config(new Loopers::NoiseScanConfig()), _fileName(path)
{
  readFile(path);
}

//=========================================================
Mechanics::NoiseMask::~NoiseMask() { delete _config; }

//=========================================================
void Mechanics::NoiseMask::writeFile(const std::string& path) const
{
	std::fstream out(path, std::ios_base::out);

  if (!out.is_open()) {
    std::string msg("[NoiseMask::writeMask]");
    msg += "unable to open file '" + path + "' for writting";
    throw std::runtime_error(msg);
  }

	for (const auto& mask : _masks) {
		auto id = mask.first;
		for (const auto& index : mask.second) {
			auto col = std::get<0>(index);
			auto row = std::get<1>(index);
			out << id << ", " << col << ", " << row << '\n';
		}
	}

	out << _config->print() << '\n';
	out.close();
	
	cout << "Wrote noise mask to '" << path << "'\n";
}

//=========================================================
void Mechanics::NoiseMask::readFile(const std::string& path)
{
  std::fstream input(path, std::ios_base::in);

  if (!input.is_open()) {
    cout << "[NoiseMask::readMask] WARNING :: Noise mask failed to open file '"
         << path << "'" << endl;
    return;
  }
  cout << "Read noise mask from '" << path << "'\n";

  std::stringstream comments;
  while (input) {
    unsigned int nsens = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    std::string line;
    std::getline(input, line);
    if (!line.size()) continue;
    if (line[0] == '#') {
      comments << line << "\n";
      continue;
    }
    std::stringstream lineStream(line);
    parseLine(lineStream, nsens, x, y);
    _masks[nsens].emplace(x, y);
  }

  if (!comments.str().empty()) parseComments(comments);
}

//=========================================================
void Mechanics::NoiseMask::parseLine(std::stringstream& line, Index& nsens,
                                     Index& x, Index& y)
{
  Index counter = 0;
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

void Mechanics::NoiseMask::maskPixel(Index sensor, Index col, Index row)
{
  _masks[sensor].emplace(col, row);
}

const Mechanics::NoiseMask::ColRowSet& Mechanics::NoiseMask::getMaskedPixels(
    Index sensor) const
{
	static const ColRowSet EMPTY;
	
  auto mask = _masks.find(sensor);
  if (mask != _masks.end()) return mask->second;
  return EMPTY;
}

const size_t Mechanics::NoiseMask::getNumMaskedPixels() const
{
  size_t n = 0;
  for (const auto& mask : _masks) n += mask.second.size();
  return n;
}
