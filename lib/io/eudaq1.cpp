/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-10
 */

#include "eudaq1.h"

#include <vector>

#include <eudaq/DetectorEvent.hh>
#include <eudaq/FileReader.hh>
#include <eudaq/PluginManager.hh>
#include <eudaq/StandardEvent.hh>

#include "storage/event.h"
#include "utils/config.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Eudaq1Reader);

namespace proteus {

// local helper functions

// determine the list of all sensor ids
static std::vector<unsigned> listIds(const ::eudaq::StandardEvent& sevent)
{
  std::vector<unsigned> ids;
  for (size_t iplane = 0; iplane < sevent.NumPlanes(); ++iplane)
    ids.push_back(sevent.GetPlane(iplane).ID());
  return ids;
}

// convert a EUDAQ StandardEvent into a Proteus Event
static void convert(const ::eudaq::StandardEvent& sevent,
                    const std::map<unsigned, Index>& mapIdIndex,
                    Event& event)
{
  uint64_t frame = sevent.GetEventNumber();
  uint64_t timestamp = sevent.GetTimestamp();
  event.clear(frame, timestamp);

  for (size_t iplane = 0; iplane < sevent.NumPlanes(); ++iplane) {
    const ::eudaq::StandardPlane& spl = sevent.GetPlane(iplane);

    // find the corresponding sensor index
    auto it = mapIdIndex.find(spl.ID());
    if (it == mapIdIndex.end())
      FAIL("unknown EUDAQ sensor id ", spl.ID());

    SensorEvent& sensorEvent = event.getSensorEvent(it->second);
    // fill hits into the sensor event
    for (unsigned i = 0; i < spl.HitPixels(); ++i) {
      auto col = spl.GetX(i);
      auto row = spl.GetY(i);
      auto value = spl.GetPixel(i);
      // StandardPlane has no per-pixel timing information
      sensorEvent.addHit(col, row, 0, value);
    }
  }
}

// automatic filetype deduction/ global registry

int Eudaq1Reader::check(const std::string& path)
{
  if (pathExtension(path) == "raw") {
    return 10;
  }
  return 0;
}

std::shared_ptr<Eudaq1Reader>
Eudaq1Reader::open(const std::string& path,
                   const toml::Value& /* unused configuration */)
{
  return std::make_shared<Eudaq1Reader>(path);
}

// FileReader proper

Eudaq1Reader::Eudaq1Reader(const std::string& path)
    : m_reader(new ::eudaq::FileReader(path)), m_thatsIt(false)
{
  // opening the reader immediately reads the first event
  // collect all beginning of run events (BOREs)
  size_t numBORE = 0;
  do {
    const ::eudaq::Event& ev = m_reader->GetEvent();
    // quit loop since we reached the first data/ non-BORE event
    if (!ev.IsBORE())
      break;
    // use BOREs to initialize the plugin manager
    if (const auto* dev = dynamic_cast<const ::eudaq::DetectorEvent*>(&ev)) {
      ::eudaq::PluginManager::Initialize(*dev);
    }
    numBORE += 1;
  } while (m_reader->NextEvent());

  if (numBORE == 0)
    INFO("no beginning-of-run events in '", path, "'");
  if (1 < numBORE)
    INFO(numBORE, " beginning-of-run events in '", path, "'");

  // at this point we have already read the first data event.
  // use it to determine available EUDAQ sensor ids and create a unique mapping
  // from sorted eudaq sensor ids to the Proteus sensor index.
  // TODO 2017-10-14 msmk: can this contain duplicate entries?
  std::vector<unsigned int> eudaqIds;
  const ::eudaq::Event& ev = m_reader->GetEvent();
  if (const auto* sev = dynamic_cast<const ::eudaq::StandardEvent*>(&ev)) {
    eudaqIds = listIds(*sev);
  } else if (const auto* dev =
                 dynamic_cast<const ::eudaq::DetectorEvent*>(&ev)) {
    eudaqIds = listIds(::eudaq::PluginManager::ConvertToStandard(*dev));
  } else {
    THROW("could not convert event ", ev.GetEventNumber(), " to StandardEvent");
  }
  std::sort(eudaqIds.begin(), eudaqIds.end());
  for (size_t i = 0; i < eudaqIds.size(); ++i)
    m_mapIdIndex[eudaqIds[i]] = i;

  INFO("read ", m_mapIdIndex.size(), " sensors from '", path, "'");
}

// required for std::unique_ptr<...> w/ forward-defined classes to work
Eudaq1Reader::~Eudaq1Reader() {}

std::string Eudaq1Reader::name() const { return "Eudaq1Reader"; }

uint64_t Eudaq1Reader::numEvents() const { return UINT64_MAX; }
size_t Eudaq1Reader::numSensors() const { return m_mapIdIndex.size(); }

void Eudaq1Reader::skip(uint64_t n)
{
  if (!m_reader->NextEvent(n))
    m_thatsIt = true;
}

bool Eudaq1Reader::read(Event& event)
{
  // due to BORE handling and id number determination, the first event must
  // be read already in the constructor. The internal event reading is
  // therefore running one event ahead. The current event requested by the
  // event loop was already read and we must instead read the next event
  // at the end of this call.

  // failure or end-of-file in previous iteration
  if (m_thatsIt)
    return false;

  const ::eudaq::Event& ev = m_reader->GetEvent();

  // reached end-of-run
  if (ev.IsEORE())
    return false;

  // event must be either already a StandardEvent or a DetectorEvent
  // that can be converted to one using the PluginManager
  if (const auto* sev = dynamic_cast<const ::eudaq::StandardEvent*>(&ev)) {
    convert(*sev, m_mapIdIndex, event);
  } else if (const auto* dev =
                 dynamic_cast<const ::eudaq::DetectorEvent*>(&ev)) {
    convert(::eudaq::PluginManager::ConvertToStandard(*dev), m_mapIdIndex,
            event);
  } else {
    FAIL("could not convert event ", ev.GetEventNumber(), " to StandardEvent");
  }

  // Read event for the next call
  if (!m_reader->NextEvent())
    m_thatsIt = true;

  return true;
}

} // namespace proteus
