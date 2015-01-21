#include "inputargs.h"

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace std;

//=========================================================
InputArgs::InputArgs() :
  _inFileRef(""),
  _outFileRef(""),    
  _inFileDUT(""),
  _outFileDUT(""),
  _results(""),
  _cfgRef(""),
  _cfgDUT(""),
  _cfgTestbeam(""),
  _command(""),
  _numEvents(0),
  _eventOffset(0),
  _noBar(false),
  _printLevel(0)
{
  _vruns.reserve(1000);
  _vruns.clear();
}

//=========================================================
InputArgs::~InputArgs(){
  _vruns.clear();
}

//=========================================================
void InputArgs::usage()
{
  cout << left;
  cout << "\nJudith usage: ./Judith -c command [arguments]" << endl;

  const unsigned int w1 = 16;
  cout << "\nCommands (required arguments, [optional arguments]):\n";
  cout << setw(w1) << "  convert"
       << " : convert KarTel data (-i, -o, [-r, -n])\n";
  cout << setw(w1) << "  synchronize"
       << " : synchronize DUT and ref. files (-i, -o, -I, -O, -r, -d, -t, [-n, -s])\n";
  cout << setw(w1) << "  applyMask"
       << " : mask sensors and noisy pixels (-i, -o, -r, -t, [-n, -s, --runs])\n";
  cout << setw(w1) << "  noiseScan"
       << " : scan for noisy pixels (-i, -r, -t, [-n, -s])\n";
  cout << setw(w1) << "  coarseAlign"
       << " : coarse align device planes (-i, -r, -t, [-n, -s])\n";
  cout << setw(w1) << "  fineAlign"
       << " : fine align device planes (-i, -r, -t, [-n, -s])\n";
  cout << setw(w1) << "  coarseAlignDUT"
       << " : coarse align DUT to ref. device (-i, -I, -r, -d, -t, [-n, -s])\n";
  cout << setw(w1) << "  fineAlignDUT"
       << " : fine align DUT planes to ref. tracks (-i, -I, -r, -d, -t, [-n, -s])\n";
  cout << setw(w1) << "  process"
       << " : generate cluster and/or tracks (-i, -o, -r, -t, [-R, -n, -s])\n";
  cout << setw(w1) << "  analysis"
       << " : analyze device events (-i, -r, -t, -R, [-n, -s])\n";
  cout << setw(w1) << "  analysisDUT"
       << " : analyze DUT events with ref. data (-i, -I, -r, -d, -t, -R, [-n, -s])\n";
  cout << endl;

  const unsigned int w2 = 13;
  cout << "Arguments:\n";
  cout << "  -i  " << setw(w2) << "--input" << " : path to data input\n";
  cout << "  -o  " << setw(w2) << "--output" << " : path to store data output\n";
  cout << "  -I  " << setw(w2) << "--inputDUT" << " : path to DUT data input\n";
  cout << "  -O  " << setw(w2) << "--outputDUT" << " : path to store DUT data output\n";
  cout << "  -R  " << setw(w2) << "--results" << " : path to store analyzed results\n";
  cout << "  -r  " << setw(w2) << "--cfgRef" << " : path to reference configuration\n";
  cout << "  -d  " << setw(w2) << "--cfgDUT" << " : path to DUT configuration\n";
  cout << "  -t  " << setw(w2) << "--cfgTestbeam" << " : path to testbeam configuration\n";
  cout << "  -n  " << setw(w2) << "--numEvents" << " : number of events to process\n";
  cout << "  -s  " << setw(w2) << "--eventOffset" << " : starting at this event\n";
  cout << "  -h  " << setw(w2) << "--help" << " : print this help message\n";
  cout << endl;

  cout << "Additional options:\n";
  cout << "  -b  " << setw(w2) << "--noBar" << " : do not print the progress bar\n";
  cout << "  -v  " << setw(w2) << "--verbose" << " : set verbosity level\n";
  cout << "      " << setw(w2) << "--runs" << " : run(s) being analyzed (single run or list). This option can be useful\n";
  cout << "      " << setw(w2) << "" << "   for merged-runs, or when the run-number is not contained in the input file.\n";
  cout << "      " << setw(w2) << "" << "   Can be a single-run, a list (comma-separated) and/or a sequence (dash-separated).\n";
  cout << "      " << setw(w2) << "" << "   Note this option is only taken into account with the 'applyMask' command.\n";
      
  cout << endl;
  cout << right;
}

//=========================================================
int InputArgs::parseArgs(int* argc, char** argv)
{
  // input argument handling
  string arg;
  
  if (*argc > 1){
    for (int i=1; i<*argc; i++ )
      {
	arg = argv[i];
	
	if ( (!arg.compare("-i") || !arg.compare("--input")) &&
	     !_inFileRef.compare("") )
	  {
	    _inFileRef = argv[++i];
	  }
	else if ( (!arg.compare("-o") || !arg.compare("--output")) &&
		  !_outFileRef.compare("") ){
	  _outFileRef = argv[++i];
	}
	else if ( (!arg.compare("-I") || !arg.compare("--inputDUT")) &&
		  !_inFileDUT.compare("") )
	  {
	    _inFileDUT = argv[++i];
	  }
	else if ( (!arg.compare("-O") || !arg.compare("--outputDUT")) &&
		  !_outFileDUT.compare("") )
	  {
	    _outFileDUT = argv[++i];
	  }
	else if ( (!arg.compare("-R") || !arg.compare("--results")) &&
		  !_results.compare("") )
	  {
	    _results = argv[++i];
	  }
	else if ( (!arg.compare("-c") || !arg.compare("--command")) &&
		  !_command.compare("") )
	  {
	    _command = argv[++i];
	  }
	else if ( (!arg.compare("-r") || !arg.compare("--cfgRef")) &&
		  !_cfgRef.compare("") )
	  {
	    _cfgRef = argv[++i];
	  }
	else if ( (!arg.compare("-d") || !arg.compare("--cfgDUT")) &&
		  !_cfgDUT.compare("") )
	  {
	    _cfgDUT = argv[++i];
	  }
	else if ( (!arg.compare("-t") || !arg.compare("--cfgTestbeam")) &&
		  !_cfgTestbeam.compare("") )
	  {
	    _cfgTestbeam = argv[++i];
	  }
	else if ( (!arg.compare("-n") || !arg.compare("--numEvents")) &&
		  !_numEvents )
	  {
	    _numEvents = atoi( argv[++i] );
	  }
	else if ( (!arg.compare("-s") || !arg.compare("--eventOffset")) &&
		  !_eventOffset )
	  {
	    _eventOffset = atoi( argv[++i] );
	  }
	else if ( (!arg.compare("-b") || !arg.compare("--noBar")) &&
		  !_noBar)
	  {
	    _noBar = true;
	  }
	else if ( (!arg.compare("-v") || !arg.compare("--printLevel")) )
	  {
	    _printLevel = atoi( argv[++i] );
	  }
	else if( !arg.compare("--runs") )
	  {
	    extractRuns(argv[++i]);
	  }
	else if ( (!arg.compare("-h")) || !arg.compare("--help"))
	  {
	    usage();
	    return 1;
	  }
	else
	  {
	    usage();
	    cout << "Unknown or duplicate argument! " << arg << "\n" << endl;
	    return 1;
	  }
      }
  }
  
  return 0;  
}

//=========================================================
void InputArgs::extractRuns(std::string par) {
  std::stringstream parst(par);
  std::string st;

  while(getline(parst,st,',')) {
    std::size_t pos = st.find("-");
    if( st.find("-") != string::npos ){ // range of runs
      int start = atoi( (st.substr(0,pos)).c_str() );
      int end = atoi( (st.substr(pos+1,st.length()-pos)).c_str() );
      for(int r=start; r<=end; r++)
	_vruns.push_back(r);
    }
    else
      _vruns.push_back(atoi(st.c_str()));
  }

  // order, remove duplicates and resize
  sort(_vruns.begin(),_vruns.end());
  std::vector<int>::iterator it = unique(_vruns.begin(),_vruns.end());
  _vruns.resize(it-_vruns.begin());  
}

//=========================================================
void InputArgs::printArgs() {
  const unsigned int w = 20;
  cout << left;
  
  if(!_inFileRef.empty())
    cout << setw(w) << "  input name" << " : " << _inFileRef << endl;
  if(!_outFileRef.empty())
    cout << setw(w) << "  output name" << " : " << _outFileRef << endl;
  if(!_inFileDUT.empty())
    cout << setw(w) << "  input name DUT" << " : " << _inFileDUT << endl;
  if(!_outFileDUT.empty())
    cout << setw(w) << "  output name DUT" << " : " << _outFileDUT << endl;
  if(!_results.empty())
    cout << setw(w) << "  results name" << " : " << _results << endl;  
  if(!_command.empty())
    cout << setw(w) << "  command" << " : " << _command << endl;
  if(!_cfgRef.empty())
    cout << setw(w) << "  cfgRef" << " : " << _cfgRef << endl;
  if(!_cfgDUT.empty())
    cout << setw(w) << "  cfgDUT" << " : " << _cfgDUT << endl;
  if(!_cfgTestbeam.empty())
    cout << setw(w) << "  cfgTestbeam" << " : " << _cfgTestbeam << endl;
  if(_numEvents!=0)
    cout << setw(w) << "  numEvents" << " : " << _numEvents << endl;
  if(_eventOffset!=0)
    cout << setw(w) << "  eventOffset" << " : " << _eventOffset << endl;
  if(_noBar)
    cout << setw(w) << "  noBar" << " : true" << endl;
  if(!_vruns.empty()){
    cout << setw(w) << "  runs" << " : ";
    std::vector<int>::const_iterator cit;
    for(cit = _vruns.begin(); cit != _vruns.end(); ++cit)
      cout << (*cit) << " ";
    cout << " (total runs = " << _vruns.size() << ")" << endl;
  }  
  cout << setw(w) << "  printLevel" << " : " << _printLevel << endl;
  cout << endl; // Space before the next cout
  cout << right;  
}

string InputArgs::getInputRef() const { return _inFileRef; }
string InputArgs::getOutputRef() const { return _outFileRef; }
string InputArgs::getInputDUT() const { return _inFileDUT; }
string InputArgs::getOutputDUT() const { return _outFileDUT; }
string InputArgs::getResults() const { return _results; }
string InputArgs::getCommand() const { return _command; }
string InputArgs::getCfgRef() const { return _cfgRef; }
string InputArgs::getCfgDUT() const { return _cfgDUT; }
string InputArgs::getCfgTestbeam() const { return _cfgTestbeam; }
ULong64_t InputArgs::getNumEvents() const { return _numEvents; }
ULong64_t InputArgs::getEventOffset() const { return _eventOffset; }
bool InputArgs::getNoBar() const { return _noBar; }
int InputArgs::getPrintLevel() const { return _printLevel; }
std::vector<int> InputArgs::getRuns() const { return _vruns; }
