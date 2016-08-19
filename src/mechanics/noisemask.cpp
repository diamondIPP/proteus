#include "noisemask.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "configparser.h"
#include "loopers/noisescan.h"
#include "mechanics/device.h"
#include "mechanics/sensor.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

using std::fstream;
using std::cout;
using std::endl;
using std::string;

Mechanics::NoiseMask::NoiseMask()
    : m_path("<TRANSIENT>")
{
}

Mechanics::NoiseMask::NoiseMask(const Loopers::NoiseScanConfig* config)
    : m_cfg(*config)
    , m_path("<TRANSIENT>")
{
}

// only needed to allow std::unique_ptr with forward-declared class
Mechanics::NoiseMask::~NoiseMask() {}

static void parseLine(std::stringstream& line, Index& nsens, Index& x, Index& y)
{
  Index counter = 0;
  while (line.good()) {
    std::string value;
    std::getline(line, value, ',');
    std::stringstream convert(value);
    switch (counter) {
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

static void parseComments(std::stringstream& comments,
                          Loopers::NoiseScanConfig& cfg)
{
  if (VERBOSE) {
    cout << "\n[NoiseMask::parseComments] input string:" << endl;
    cout << comments.str() << endl;
  }

  // remove '#' characters from input string
  std::string str(comments.str());
  str.erase(std::remove(str.begin(), str.end(), '#'), str.end());

  // create temporary file to write metadata
  char nameBuff[] = "/tmp/noiseMask.XXXXXX";
  int fd = mkstemp(nameBuff);
  if (fd == -1) {
    std::string err("[NoiseMask::parseComments] ERROR can't create temp file");
    err += string(nameBuff) + "'";
    throw err.c_str();
  }
  if (VERBOSE)
    cout << "temp file '" << nameBuff << "' created OK" << endl;
  write(fd, str.c_str(), strlen(str.c_str()));
  close(fd);

  // use ConfigParser to parse contents
  ConfigParser parser(nameBuff);
  if (VERBOSE)
    parser.print();

  for (unsigned int i = 0; i < parser.getNumRows(); i++) {
    const ConfigParser::Row* row = parser.getRow(i);

    if (row->isHeader)
      continue;
    if (row->header.compare("Noise Scan"))
      continue;

    if (!row->key.compare("runs")) {
      std::vector<int> runs;
      ConfigParser::valueToVec(row->value, runs);
      cfg.setRuns(runs);
      runs.clear();
    } else if (!row->key.compare("max factor"))
      cfg.setMaxFactor(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("max occupancy"))
      cfg.setMaxOccupancy(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("bottom x"))
      cfg.setBottomLimitX(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("upper x"))
      cfg.setUpperLimitX(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("bottom y"))
      cfg.setBottomLimitY(ConfigParser::valueToNumerical(row->value));
    else if (!row->key.compare("upper y"))
      cfg.setUpperLimitY(ConfigParser::valueToNumerical(row->value));
    else {
      cout << "[NoiseMask::parseComments] WARNING can't parse row with key '"
           << row->key << endl;
    }
  }

  // delete temporary file
  unlink(nameBuff);
}

Mechanics::NoiseMask Mechanics::NoiseMask::fromFile(const std::string& path)
{
  NoiseMask noise;
  std::fstream input(path, std::ios_base::in);

  if (!input) {
    cout << "[NoiseMask::readMask] WARNING :: Noise mask failed to open file '"
         << path << "'" << endl;
    return noise;
  }
  cout << "Read noise mask from '" << path << "'\n";

  std::stringstream comments;
  while (input) {
    unsigned int nsens = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    std::string line;
    std::getline(input, line);
    if (!line.size())
      continue;
    if (line[0] == '#') {
      comments << line << "\n";
      continue;
    }
    std::stringstream lineStream(line);
    parseLine(lineStream, nsens, x, y);
    noise.maskPixel(nsens, x, y);
  }

  if (!comments.str().empty())
    parseComments(comments, noise.m_cfg);

  noise.m_path = path;
}

void Mechanics::NoiseMask::writeFile(const std::string& path) const
{
  std::fstream out(path, std::ios_base::out);

  if (!out.is_open()) {
    std::string msg("[NoiseMask::writeMask]");
    msg += "unable to open file '" + path + "' for writting";
    throw std::runtime_error(msg);
  }

  for (auto it = m_maskedPixels.begin(); it != m_maskedPixels.end(); ++it) {
    Index sensorId = it->first;
    const ColumnRowSet& pixels = it->second;
    for (auto jt = pixels.begin(); jt != pixels.end(); ++jt) {
      auto col = std::get<0>(*jt);
      auto row = std::get<1>(*jt);
      out << sensorId << ", " << col << ", " << row << '\n';
    }
  }

  out << m_cfg.print() << '\n';
  out.close();

  cout << "Wrote noise mask to '" << path << "'\n";
}

void Mechanics::NoiseMask::maskPixel(Index sensorId, Index col, Index row)
{
  m_maskedPixels[sensorId].emplace(col, row);
}

const Mechanics::NoiseMask::ColumnRowSet&
Mechanics::NoiseMask::getMaskedPixels(Index sensorId) const
{
  static const ColumnRowSet EMPTY;

  auto mask = m_maskedPixels.find(sensorId);
  if (mask != m_maskedPixels.end())
    return mask->second;
  return EMPTY;
}

const size_t Mechanics::NoiseMask::getNumMaskedPixels() const
{
  size_t n = 0;
  for (auto it = m_maskedPixels.begin(); it != m_maskedPixels.end(); ++it)
    n += it->second.size();
  return n;
}
