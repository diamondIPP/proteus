#ifndef INPUTARGS_H
#define INPUTARGS_H

#include <Rtypes.h>
#include <string>
#include <vector>

class InputArgs {
 public:
  InputArgs();
  ~InputArgs();
  
  int parseArgs(int* argc, char** argv);
  void usage();
  void printArgs();
    
  std::string getInputRef() const;
  std::string getOutputRef() const;
  std::string getInputDUT() const;
  std::string getOutputDUT() const;
  std::string getResults() const;
  std::string getCommand() const;
  std::string getCfgRef() const;
  std::string getCfgDUT() const;
  std::string getCfgTestbeam() const;
  ULong64_t getNumEvents() const;
  ULong64_t getEventOffset() const;
  bool getNoBar() const;
  int getPrintLevel() const;
  std::vector<int> getRuns() const;

 private:
  void extractRuns(std::string);

 private:
  std::string _inFileRef;
  std::string _outFileRef;
  std::string _inFileDUT;
  std::string _outFileDUT;
  std::string _results;
  std::string _cfgRef;
  std::string _cfgDUT;
  std::string _cfgTestbeam;
  std::string _command;
  ULong64_t _numEvents;
  ULong64_t _eventOffset;
  bool _noBar;
  int _printLevel;

  std::vector<int> _vruns;
  
};

#endif // INPUTARGS_H
