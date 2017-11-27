/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-10
 */

#include "eudaq2.h"

#include <cassert>
#include <vector>

#include <eudaq/FileReader.hh>
#include <eudaq/StdEventConverter.hh>

#include "storage/event.h"
#include "utils/config.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Eudaq2Reader);

// local helper functions

// convert eudaq Event to StandardEvent w/ error handling
static std::shared_ptr<const eudaq::StandardEvent>
toEudaqStandard(std::shared_ptr<const eudaq::Event> event)
{
  assert(event && "event must be non-null");

  auto sevent = eudaq::StandardEvent::MakeShared();
  if (!eudaq::StdEventConverter::Convert(event, sevent, nullptr))
    THROW("could not convert event ", event->GetEventN(), " to StandardEvent");
  return sevent;
}

// determine the list of all sensor ids
static std::vector<unsigned>
listIds(std::shared_ptr<const eudaq::StandardEvent> sevent)
{
  assert(sevent && "sevent must be non-null");

  std::vector<unsigned> ids;
  for (size_t iplane = 0; iplane < sevent->NumPlanes(); ++iplane)
    ids.push_back(sevent->GetPlane(iplane).ID());
  return ids;
}

// convert a EUDAQ StandardEvent into a Proteus Event
static void convert(std::shared_ptr<const eudaq::StandardEvent> sevent,
                    const std::map<unsigned, Index>& mapIdIndex,
                    Storage::Event& event)
{
  assert(sevent && "sevent must be non-null");

  uint64_t frame = sevent->GetEventN();
  uint64_t timestamp = sevent->GetTimestampBegin();
  event.clear(frame, timestamp);

  for (size_t iplane = 0; iplane < sevent->NumPlanes(); ++iplane) {
    const eudaq::StandardPlane& spl = sevent->GetPlane(iplane);

    // find the corresponding sensor index
    auto it = mapIdIndex.find(spl.ID());
    if (it == mapIdIndex.end())
      FAIL("unknown EUDAQ sensor id ", spl.ID());

    Storage::SensorEvent& sensorEvent = event.getSensorEvent(it->second);
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

int Io::Eudaq2Reader::check(const std::string& path)
{
  if (Utils::Config::pathExtension(path) == "raw")
    return 10;
  return 0;
}

std::shared_ptr<Io::Eudaq2Reader>
Io::Eudaq2Reader::open(const std::string& path,
                       const toml::Value& /* unused configuration */)
{
  return std::make_shared<Eudaq2Reader>(path);
}

// FileReader proper

Io::Eudaq2Reader::Eudaq2Reader(const std::string& path)
    : m_reader(eudaq::Factory<eudaq::FileReader>::MakeUnique(
          eudaq::str2hash("native"), path))
{
  // read all beginning of run events (BOREs)
  size_t numBORE = 0;
  while (true) {
    m_event = m_reader->GetNextEvent();
    if (!m_event)
      THROW("could not read event ", numBORE, " from '", path, "'");
    // quit loop since we reached the first data/ non-BORE event
    if (!m_event->IsBORE())
      break;
    numBORE += 1;
  };
  if (numBORE == 0)
    INFO("no beginning-of-run events in '", path, "'");
  if (1 < numBORE)
    INFO(numBORE, " beginning-of-run events in '", path, "'");

  // at this point we have already read the first data event.
  // use it to determine available EUDAQ sensor ids and create a unique mapping
  // from sorted eudaq sensor ids to the Proteus sensor index.
  // TODO 2017-10-14 msmk: can this contain duplicate entries?
  std::vector<unsigned int> eudaqIds = listIds(toEudaqStandard(m_event));
  std::sort(eudaqIds.begin(), eudaqIds.end());
  for (size_t i = 0; i < eudaqIds.size(); ++i)
    m_mapIdIndex[eudaqIds[i]] = i;

  INFO("read ", m_mapIdIndex.size(), " sensors from '", path, "'");
}

// required for std::unique_ptr<...> w/ forward-defined classes to work
Io::Eudaq2Reader::~Eudaq2Reader() {}

std::string Io::Eudaq2Reader::name() const { return "Eudaq2Reader"; }

uint64_t Io::Eudaq2Reader::numEvents() const { return UINT64_MAX; }
size_t Io::Eudaq2Reader::numSensors() const { return m_mapIdIndex.size(); }

void Io::Eudaq2Reader::skip(uint64_t n)
{
  for (; 0 < n; --n) {
    m_event = m_reader->GetNextEvent();
    if (!m_event)
      break;
  }
}

bool Io::Eudaq2Reader::read(Storage::Event& event)
{
  // due to BORE handling and id number determination, the first event must
  // be read already in the constructor. The internal event reading is
  // therefore running one event ahead. The current event requested by the
  // event loop was already read and we must instead read the next event
  // at the end of this call.

  // read error or reached end-of-run
  if (!m_event)
    return false;
  if (m_event->IsEORE())
    return false;

  convert(toEudaqStandard(m_event), m_mapIdIndex, event);

  // Read event for the next call
  m_event = m_reader->GetNextEvent();
  return true;
}
