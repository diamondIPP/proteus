#include "timepix3.h"
#include <sstream>
#include <string>

#include "storage/event.h"
#include "storage/sensorevent.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Timepix3)

Io::Timepix3Reader::Timepix3Reader(const std::string& path)
    : m_file()
    , m_syncTime(0)
    , m_prevTime(0)
    , m_clearedHeader(false)
    , m_globalCurrentTime(0)
{
  INFO("Timepix3Reader::constructor");

  if (path.empty()) {
    FAIL("Empty file path");
  }

  m_file = std::ifstream(path, std::ios::binary);
  if (!m_file.is_open()) {
    FAIL("could not open '", path, "' to read.");
  }
  INFO("Timepix3Reader - opened file '", path, "' for reading.");
  uint32_t headerID = 0;
  if (!m_file.read(reinterpret_cast<char*>(&headerID), sizeof headerID)) {
    FAIL("Could not read header.");
  } else {
    INFO("Header ID: ", headerID);
  }

  uint32_t headerSize = 0;
  if (!m_file.read(reinterpret_cast<char*>(&headerSize), sizeof headerSize)) {
    FAIL("Cannot read header size for device.");
  } else {
    INFO("Header size: ", headerSize);
  }

  INFO(
      "Timepix3Reader - read header size for device, skipping rest of header.");
  // Skip the full header:
  m_file.seekg(headerSize);
}

Io::Timepix3Reader::~Timepix3Reader() { INFO("Timepix3Reader::deconstructor"); }

std::string Io::Timepix3Reader::name() const { return "Timepix3Reader"; }

void Io::Timepix3Reader::skip(uint64_t n) {
  INFO("Timepix3Reader::skip");

  Storage::SensorEvent evt(0);
  for(uint64_t i = 0; i < n; i++) {
      if(!getSensorEvent(evt)) {
          return;
      }
      evt.clear(0,0);
  }
}

bool Io::Timepix3Reader::read(Storage::Event& event) {

    INFO("Timepix3Reader::read");
    Storage::SensorEvent sensorEvent(0);
    bool status = getSensorEvent(sensorEvent);
    INFO("Event with ", sensorEvent.numHits(), " hits");
    event.setSensorData(0, std::move(sensorEvent));

    return status;
}

bool Io::Timepix3Reader::getSensorEvent(Storage::SensorEvent& sensorEvent) {

  double event_length_time = 0.1;

  // Count pixels read in this "frame"
  int npixels = 0;

  // Read a block of data until we reach the EOF or have enough events:
  ULong64_t pixdata = 0;

  // Read 64-bit chunks of data
  while (m_file.read(reinterpret_cast<char*>(&pixdata), sizeof pixdata)) {

    std::stringstream s;
    s << std::hex << pixdata << std::dec;
    DEBUG("data: 0x", s.str());

    // Get the header (first 4 bits) and do things depending on what it is
    // 0x4 is the "heartbeat" signal, 0xA and 0xB are pixel data
    const UChar_t header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;

    if (header == 0x7) {
      DEBUG("Header 0x7: 'Config Acknowledge'");
    }

    if (header == 0x4) {
      DEBUG("Header 0x4: 'Heartbeat'");
      // The 0x4 header tells us that it is part of the timestamp
      // There is a second 4-bit header that says if it is the most or least
      // significant part of the timestamp
      const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

      // This is a bug fix. There appear to be errant packets with garbage data
      // - source to be tracked down.
      // Between the data and the header the intervening bits should all be 0,
      // check if this is the case
      const UChar_t intermediateBits =
          ((pixdata & 0x00FF000000000000) >> 48) & 0xFF;
      if (intermediateBits != 0x00)
        continue;

      // 0x4 is the least significant part of the timestamp
      if (header2 == 0x4) {
        DEBUG("            'Heartbeat' LSB");
        // The data is shifted 16 bits to the right, then 12 to the left in
        // order to match the timestamp format (net 4 right)
        m_syncTime = (m_syncTime & 0xFFFFF00000000000) +
                     ((pixdata & 0x0000FFFFFFFF0000) >> 4);
      }

      // 0x5 is the most significant part of the timestamp
      if (header2 == 0x5) {
        DEBUG("            'Heartbeat' MSB");
        // The data is shifted 16 bits to the right, then 44 to the left in
        // order to match the timestamp format (net 28 left)
        m_syncTime = (m_syncTime & 0x00000FFFFFFFFFFF) +
                     ((pixdata & 0x00000000FFFF0000) << 28);
        if (!m_clearedHeader && (double)m_syncTime / (4096. * 40000000.) < 6.) {
          DEBUG("Cleared header");
          m_clearedHeader = true;
        }
        DEBUG("            'Heartbeat' timestamp: ", (double)m_syncTime / (4096. * 40000000.));
      }
    }

    if (!m_clearedHeader)
      continue;

    // Header 0x06 and 0x07 are the start and stop signals for power pulsing
    if (header == 0x0) {
      DEBUG("Header 0x0: 'Power Pulsing'");
      /*
      // Get the second part of the header
      const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

      // New implementation of power pulsing signals from Adrian
      if (header2 == 0x6) {
        DEBUG("Header two = 0x6.");
        const uint64_t time((pixdata & 0x0000000FFFFFFFFF) << 12);

        const uint64_t controlbits =
            ((pixdata & 0x00F0000000000000) >> 52) & 0xF;

        const uint64_t powerOn = ((controlbits & 0x2) >> 1);
        const uint64_t shutterClosed = ((controlbits & 0x1));

        // Stop looking at data if the signal is after the current event window
        // (and rewind the file
        // reader so that we start with this signal next event)

        // FIXME re-implement!
      }
      */
    }

    // Header 0xA and 0xB indicate pixel data
    if (header == 0xA || header == 0xB) {
      DEBUG("Header 0xA | 0xB: 'Pixel Data'");
      // Decode the pixel information from the relevant bits
      const UShort_t dcol = ((pixdata & 0x0FE0000000000000) >> 52);
      const UShort_t spix = ((pixdata & 0x001F800000000000) >> 45);
      const UShort_t pix = ((pixdata & 0x0000700000000000) >> 44);
      const UShort_t col = (dcol + pix / 4);
      const UShort_t row = (spix + (pix & 0x3));

      // Get the rest of the data from the pixel
      const UShort_t pixno = col * 256 + row;
      const UInt_t data = ((pixdata & 0x00000FFFFFFF0000) >> 16);
      const unsigned int tot = (data & 0x00003FF0) >> 4;
      const uint64_t spidrTime(pixdata & 0x000000000000FFFF);
      const uint64_t ftoa(data & 0x0000000F);
      const uint64_t toa((data & 0x0FFFC000) >> 14);

      // Calculate the timestamp.
      long long int time =
          (((spidrTime << 18) + (toa << 4) + (15 - ftoa)) << 8) +
          (m_syncTime & 0xFFFFFC0000000000);
      DEBUG("Calculated timestamp: ", time, ", ",
            ((double)time / (4096. * 40000000.)));

      // Add the timing offset from the coniditions file (if any)
      // FIXME re-add if necessary

      // The time from the pixels has a maximum value of ~26 seconds. We compare
      // the pixel time
      // to the "heartbeat" signal (which has an overflow of ~4 years) and check
      // if the pixel
      // time has wrapped back around to 0

      // If the counter overflow happens before reading the new heartbeat
      while (m_syncTime - time > 0x0000020000000000) {
        time += 0x0000040000000000;
        DEBUG("Adjusting timestamp to account for pixel timestamp overflow");
      }

      // If events are loaded based on time intervals, take all hits where the
      // time is within this window

      // Stop looking at data if the pixel is after the current event window
      // (and rewind the file
      // reader so that we start with this pixel next event)
      if (event_length_time > 0. &&
          ((double)time / (4096. * 40000000.)) >
              (m_globalCurrentTime + event_length_time)) {
        DEBUG("Configured event length reached: ",
              ((double)time / (4096. * 40000000.)), " > ",
              (m_globalCurrentTime + event_length_time));
        m_file.seekg(-1 * sizeof(pixdata), std::ios_base::cur);
        break;
      }

      // Otherwise create a new pixel object
      Storage::Hit& hit = sensorEvent.addHit(col, row, ((float)time / (4096. * 40000000.)), tot);

      DEBUG("Pixel #", npixels, ": ", hit);
      npixels++;
      m_prevTime = time;
    }

    if (m_file.eof()) {
      INFO("Reached end of file. Fin.");
      return false;
    }
  }

  // Increment global time stamp:
  m_globalCurrentTime += event_length_time;

  return true;
}
