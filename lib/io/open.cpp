#include "open.h"

#include <functional>
#include <set>
#include <vector>

#ifdef PT_USE_EUDAQ1
#include "io/eudaq1.h"
#endif
#ifdef PT_USE_EUDAQ2
#include "io/eudaq2.h"
#endif
#include "io/rceroot.h"
#include "io/timepix3.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Io);

// Reader format registry
//
// Two methods need to be provided for each file format:
//
// *   a `check(...)` function that takes the input path and returns a score.
// *   an `open(...)` function that takes the input path and a configuration
//     object, opens the file, and returns a `shared_ptr` to the resulting
//     `EventReader`
//
// The `check` function is used to determine if a path could potentially belong
// to a file of the selected format. A returned score above 0 should indicate a
// possible match. The file is then tried to be opened with all matched
// formats starting from the highest score.
namespace Io {
namespace {

struct Format {
  using EventReaderPtr = std::shared_ptr<Loop::Reader>;

  const char* name;
  std::function<int(const std::string&)> check;
  std::function<EventReaderPtr(const std::string&, const toml::Value&)> open;
};
struct ScoredFormat {
  const Format& format;
  int score;
};
// A format with a higher score should come before one with a lower score
bool operator<(const ScoredFormat& a, const ScoredFormat& b)
{
  return b.score < a.score;
}

// The global list of available readers that is considered for the automatic
// file type deduction when using `openRead(...)`
//
// Using a static list, to which all readers must be manually added, is not an
// elegant solution. It would be nicer for Readers to register automatically to
// the list in their own code. Unfortunately, this must happen before any code
// in the main function is executed. In principle, this could be done using
// static global variables that will be initialized automatically. However, this
// does not work for static libraries for which the unused static variables will
// be removed by the linker and their constructors will never be called.
//
// This version requires manual registration, but just works (tm).
static std::vector<Format> s_formats = {
#ifdef PT_USE_EUDAQ1
    {"eudaq1", Eudaq1Reader::check, Eudaq1Reader::open},
#endif
#ifdef PT_USE_EUDAQ2
    {"eudaq2", Eudaq2Reader::check, Eudaq2Reader::open},
#endif
    {"rceroot", RceRootReader::check, RceRootReader::open},
    {"timepix3", Timepix3Reader::check, Timepix3Reader::open}};

} // namespace
} // namespace Io

std::shared_ptr<Loop::Reader> Io::openRead(const std::string& path,
                                           const toml::Value& cfg)
{
  std::set<ScoredFormat> scoredFormats;

  DEBUG("supported reader formats:");
  for (const Format& f : s_formats)
    DEBUG("  ", f.name);

  // find potential readers to open the file, i.e. score > 0.
  // the set automatically sorts them with the most probably format first.
  for (const auto& fmt : s_formats) {
    ScoredFormat sf = {fmt, fmt.check(path)};
    if (0 < sf.score)
      scoredFormats.emplace(std::move(sf));
  }

  // start w/ highest score format until the file is opened or list is exhausted
  for (const auto& sf : scoredFormats) {
    try {
      auto reader = sf.format.open(path, cfg);
      if (reader) {
        return reader;
      }
    } catch (const std::exception& e) {
      ERROR(e.what());
    }
    INFO("could not open '", path, "' with format '", sf.format.name, "'");
  }

  // there are either no possible readers or all readers have failed
  if (scoredFormats.empty()) {
    FAIL("could not determine file format for '", path, "'");
  } else {
    FAIL("could not open '", path, "'");
  }

  // this should never be reached but makes the compiler happy
  return nullptr;
}
