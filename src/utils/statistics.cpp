#include <sstream>

#include "statistics.h"

Utils::EventStatistics::EventStatistics()
    : events(0)
    , eventsWithHits(0)
    , eventsWithClusters(0)
    , eventsWithTracks(0)
    , eventsWithOneTrack(0)
{
}

void Utils::EventStatistics::fill(uint64_t nHits,
                                  uint64_t nClusters,
                                  uint64_t nTracks)
{
  events += 1;
  eventsWithHits += (0 < nHits) ? 1 : 0;
  eventsWithClusters += (0 < nClusters) ? 1 : 0;
  eventsWithTracks += (0 < nTracks) ? 1 : 0;
  eventsWithOneTrack += (nTracks == 1) ? 1 : 0;
  hits.fill(nHits);
  clusters.fill(nClusters);
  tracks.fill(nTracks);
}

std::string Utils::EventStatistics::str(const std::string& prefix) const
{
  std::ostringstream os;

  os << prefix << "events: " << events << '\n';
  os << prefix << "events w/ hits: " << eventsWithHits << '\n';
  os << prefix << "events w/ clusters: " << eventsWithClusters << '\n';
  os << prefix << "events w/ tracks: " << eventsWithTracks << '\n';
  os << prefix << "events w/ one track: " << eventsWithOneTrack << '\n';
  os << prefix << "hits/event: " << hits << '\n';
  os << prefix << "clusters/event: " << clusters << '\n';
  os << prefix << "tracks/event: " << tracks << '\n';

  return os.str();
}
