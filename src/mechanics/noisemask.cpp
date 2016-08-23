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
#include "utils/logger.h"

using Utils::logger;

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
    throw std::runtime_error("NoiseMask: could not parse line");
}

static void parseComments(std::stringstream& comments,
                          Loopers::NoiseScanConfig& cfg)
{
  using std::cout;

  DEBUG("parseComments input string:\n", comments.str(), '\n');

  // remove '#' characters from input string
  std::string str(comments.str());
  str.erase(std::remove(str.begin(), str.end(), '#'), str.end());

  // create temporary file to write metadata
  char nameBuff[] = "/tmp/noiseMask.XXXXXX";
  int fd = mkstemp(nameBuff);
  if (fd == -1) {
    std::string msg("NoiseMask: Can't create temp file '");
    msg += nameBuff;
    msg += '\'';
    throw std::runtime_error(msg);
  }

  DEBUG("Created temp file '", nameBuff, "'n");

  write(fd, str.c_str(), strlen(str.c_str()));
  close(fd);

  // use ConfigParser to parse contents
  ConfigParser parser(nameBuff);
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
      ERROR("Can't parse row, key='", row->key, "'\n");
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
    ERROR("Failed to open file '", path, "'\n");
    return noise;
  }
  INFO("Read noise mask from '", path, "'\n");

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

  return noise;
}

Mechanics::NoiseMask::NoiseMask(const Loopers::NoiseScanConfig* config)
    : m_cfg(*config)
{
}

void Mechanics::NoiseMask::writeFile(const std::string& path) const
{
  std::fstream out(path, std::ios_base::out);

  if (!out.is_open()) {
    std::string msg("NoiseMask: Can't open file'");
    msg += path;
    msg += '\'';
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

  INFO("Wrote noise mask to '", path, "'\n");
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

void Mechanics::NoiseMask::print(std::ostream& os,
                                 const std::string& prefix) const
{
  os << prefix << "Pixel Masks:\n";

  auto ipixs = m_maskedPixels.begin();
  for (; ipixs != m_maskedPixels.end(); ++ipixs) {
    Index i = ipixs->first;
    const ColumnRowSet& pixs = ipixs->second;

    std::string indent = prefix + "  ";

    if (pixs.empty())
      continue;

    os << indent << "Sensor " << i << ":\n";

    auto cr = pixs.begin();
    for (; cr != pixs.end(); ++cr) {
      os << indent << "  col=" << cr->first << ", row=" << cr->second << '\n';
    }
  }
}
