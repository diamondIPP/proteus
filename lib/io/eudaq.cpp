/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-10
 */

#include "eudaq.h"

#include <vector>

#include <eudaq/DetectorEvent.hh>
#include <eudaq/FileReader.hh>
#include <eudaq/PluginManager.hh>
#include <eudaq/StandardEvent.hh>

#include "storage/event.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(EudaqReader);

// determine the list of all sensor ids
static std::vector<unsigned> listIds(const eudaq::StandardEvent& sev)
{
  std::vector<unsigned> ids;
  for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane)
    ids.push_back(sev.GetPlane(iplane).ID());
  return ids;
}

// convert a EUDAQ StandardEvent into a Proteus Event
static void convert(const eudaq::StandardEvent& sev,
                    const std::map<unsigned, Index>& mapIdIndex,
                    Storage::Event& event)
{
  uint64_t frame = sev.GetEventNumber();
  uint64_t timestamp = sev.GetTimestamp();
  event.clear(frame, timestamp);

  for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {
    const eudaq::StandardPlane& spl = sev.GetPlane(iplane);

    // find the corresponding sensor index
    auto it = mapIdIndex.find(spl.ID());
    if (it == mapIdIndex.end())
      FAIL("unknown EUDAQ sensor id ", spl.ID());

    // fill hits into the sensor event
    Storage::SensorEvent& sensorEvent = event.getSensorEvent(it->second);
    // StandarPlane has no timestamp on its own
    sensorEvent.clear(spl.TLUEvent(), timestamp);
    for (unsigned i = 0; i < spl.HitPixels(); ++i) {
      auto col = spl.GetX(i);
      auto row = spl.GetY(i);
      auto value = spl.GetPixel(i);
      // StandardPlane has no per-pixel timing information
      sensorEvent.addHit(col, row, 0, value);
    }
  }
}

Io::EudaqReader::EudaqReader(const std::string& path)
    : m_reader(new eudaq::FileReader(path)), m_thatsIt(false)
{
  // read all beginning of run events (BOREs)
  size_t numBORE = 0;
  do {
    const auto& ev = m_reader->GetEvent();
    // quit loop since we reached the first data/ non-BORE event
    if (!ev.IsBORE())
      break;
    // otherwise, use BORE to initialize the PluginManager
    if (const auto* dev = dynamic_cast<const eudaq::DetectorEvent*>(&ev))
      eudaq::PluginManager::Initialize(*dev);
  } while (m_reader->NextEvent());

  if (numBORE == 0)
    INFO("'", path, "' has no beginning-of-run events");
  if (1 < numBORE)
    INFO("'", path, "' has ", numBORE, " beginning-of-run events");

  // use first event to determine available EUDAQ sensor ids
  const auto& ev = m_reader->GetEvent();
  // event must be either already a StandardEvent or a DetectorEvent that can be
  // converted to one using the PluginManager
  std::vector<unsigned> eudaqIds;
  if (const auto* sev = dynamic_cast<const eudaq::StandardEvent*>(&ev)) {
    eudaqIds = listIds(*sev);
  } else if (const auto* dev = dynamic_cast<const eudaq::DetectorEvent*>(&ev)) {
    eudaqIds = listIds(eudaq::PluginManager::ConvertToStandard(*dev));
  } else {
    THROW("could not convert event ", ev.GetEventNumber(), " to StandardEvent");
  }

  // use sorted sensor ids for a unique mapping to the Proteus sensor index
  // TODO 2017-10-14 msmk: can this contain double entries?
  std::sort(eudaqIds.begin(), eudaqIds.end());
  for (size_t i = 0; i < eudaqIds.size(); ++i)
    m_mapIdIndex[eudaqIds[i]] = i;

  INFO("read ", m_mapIdIndex.size(), " sensors from '", path, "'");
}

// required for std::unique_ptr<...> w/ forward-defined classes to work
Io::EudaqReader::~EudaqReader() {}

std::string Io::EudaqReader::name() const { return "EudaqReader"; }

uint64_t Io::EudaqReader::numEvents() const { return UINT64_MAX; }
size_t Io::EudaqReader::numSensors() const { return m_mapIdIndex.size(); }

void Io::EudaqReader::skip(uint64_t n)
{
  if (!m_reader->NextEvent(n))
    m_thatsIt = true;
}

bool Io::EudaqReader::read(Storage::Event& event)
{
  if (m_thatsIt)
    return false;

  // due to BORE handling and id number determination, the first event must
  // be read already in the constructor. The internal event reading is
  // therefore running one event ahead. The current event requested by the
  // event loop was already read and we must instead read the next event
  // at the end of this call.

  const eudaq::Event& ev = m_reader->GetEvent();

  // reached end-of-run
  if (ev.IsEORE())
    return false;

  // event must be either already a StandardEvent or a DetectorEvent that can be
  // converted to one using the PluginManager
  if (const auto* sev = dynamic_cast<const eudaq::StandardEvent*>(&ev)) {
    convert(*sev, m_mapIdIndex, event);
  } else if (const auto* dev = dynamic_cast<const eudaq::DetectorEvent*>(&ev)) {
    convert(eudaq::PluginManager::ConvertToStandard(*dev), m_mapIdIndex, event);
  } else {
    FAIL("could not convert event ", ev.GetEventNumber(), " to StandardEvent");
  }

  // Read event for the next call
  if (!m_reader->NextEvent())
    m_thatsIt = true;

  return true;
}
