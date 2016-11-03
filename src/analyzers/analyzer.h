/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08-24
 */

#ifndef PT_ANALYZER_H
#define PT_ANALYZER_H

#include <cstdint>
#include <string>

namespace Storage {
class Event;
}

namespace Analyzers {

class Analyzer {
public:
  virtual ~Analyzer();
  virtual std::string name() const = 0;
  virtual void analyze(const Storage::Event& event) = 0;
  virtual void finalize() = 0;
};

} // namespace Analyzers

#endif // PT_ANALYZER_H
