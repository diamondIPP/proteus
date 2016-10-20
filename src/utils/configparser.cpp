#include "configparser.h"

#include <cassert>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdexcept>

using std::string;
using std::cout;
using std::endl;

//=========================================================
ConfigParser::ConfigParser(const char* filePath,
			   int printLevel) :
  _filePath(filePath),
  _numRows(0),
  _printLevel(printLevel)
{
  _inputFile.open(_filePath);

  if(!_inputFile.is_open())
    throw std::runtime_error("ConfigParser: failed to open file '" +
                             std::string(filePath) + "' to read");

  _currentHeader = "";
  _currentValue  = "";
  _currentKey    = "";
  _lineBuffer    = "";

  parseContents(_inputFile);
  
  _inputFile.close();
}

//=========================================================
void ConfigParser::parseContents(std::ifstream& input) {
  while(!readNextLine(input)) {
    if (!parseForHeader())
      {
	Row row;
	row.header = _currentHeader;
	row.key = "";
	row.value = "";
	row.isHeader = true;
	_parsedContents.push_back(row);
	_numRows++;
      }
    // Look for a link to another configuration file
    else if( !parseForLink() )
      {
	std::ifstream linked;
	linked.open(_currentValue.c_str());
    if (!linked.is_open())
      throw std::runtime_error(
          "ConfigParser: unable to opened linked configuration '" +
          _currentValue + '\'');
    parseContents(linked);
      }
    else if (!parseForKey())
      {
	Row row;
	row.header = _currentHeader;
	row.key    = _currentKey;
	row.value  = _currentValue;
	row.isHeader = false;
	_parsedContents.push_back(row);
	_numRows++;
      }
  }
}

//=========================================================
int ConfigParser::readNextLine(std::ifstream& intput) {
  // Search for a non commented line
  while (intput.good())
  {
    string buffer;
    getline(intput, buffer);

    // Find the end of the line (eol or start of a comment)
    size_t end = buffer.find_first_of('#');
    if (end == string::npos) end = buffer.length();

    // If the first non-blank character
    size_t first = buffer.find_first_not_of(" \t\r\n");
    if (first == string::npos) first = buffer.length();

    // If the first valid character is the end, this line is empty
    if (first >= end) continue;

    // The line in buffer is good, make a stream from it
    _lineBuffer = buffer.substr(first, end - first);

    return 0;
  }

  return -1;
}

//=========================================================
int ConfigParser::parseForHeader() {
  size_t start = _lineBuffer.find_first_of('[');
  size_t end   = _lineBuffer.find_first_of(']');
  
  if (!checkRange(start, end)) return -1;
  
  _currentHeader = _lineBuffer.substr(start+1, end-start-1);

  return 0;
}

//=========================================================
bool ConfigParser::checkRange(size_t start, size_t end) {
  if(start == string::npos || end == string::npos || start > end)
    return false;
  else
    return true;
}

//=========================================================
int ConfigParser::parseForKey() {
  size_t separator = _lineBuffer.find_first_of(':');
  
  if (separator == string::npos) return -1;
  
  string keyString = _lineBuffer.substr(0, separator);
  
  size_t keyStart = keyString.find_first_not_of(" \t\r\n");
  size_t keyEnd   = keyString.find_last_not_of(" \t\r\n");
  
  if (!checkRange(keyStart, keyEnd))
    _currentKey = "";
  else
    _currentKey   = keyString.substr(keyStart, keyEnd + 1 - keyStart);
  
  string valueString = _lineBuffer.substr(separator + 1);
  
  size_t valueStart = valueString.find_first_not_of(" \t\r\n");
  size_t valueEnd   = valueString.find_last_not_of(" \t\r\n");
  
  if (!checkRange(valueStart, valueEnd))
    _currentValue = "";
  else
    _currentValue = valueString.substr(valueStart, valueEnd - valueStart + 1);

  return 0;
}

//=========================================================
int ConfigParser::parseForLink()
{
  if (parseForKey()) return -1;
  if (_currentKey.compare("LINK")) return -1;
  return 0;
}


//=========================================================
void ConfigParser::print(){
  std::cout << "[ConfigParser::print]" << std::endl;
  std::cout << "  - filePath = " << _filePath << std::endl;
  std::cout << "  - nRows = " << _numRows    
	    << ". Showing all lines according to format <header,key,value,isHeader>"
	    << std::endl;
  const int w = _parsedContents.size() < 100 ? 2 : 3;
  std::vector<Row>::const_iterator cit; int cnt=0;
  for(cit=_parsedContents.begin(); cit!=_parsedContents.end(); ++cit, cnt++){
    Row row = (*cit);
    std::string blank = row.isHeader ? "" : "     ";
    std::cout  << "     [" << std::setw(w) << cnt << "] " << blank << " <"
	       << row.header << " , " << row.key  << " , " << row.value
	       << " , " << row.isHeader << ">" << std::endl;
  }  
}

//=========================================================
const ConfigParser::Row* ConfigParser::getRow(unsigned int n) const {
  assert(n < _numRows &&
         "ConfigParser: row index outside range");
  return &(_parsedContents.at(n));
}

//=========================================================
double ConfigParser::valueToNumerical(const string& value) {
  std::stringstream ss;
  ss.str(value);
  double num = 0;
  ss >> num;
  return num;
}

//=========================================================
bool ConfigParser::valueToLogical(const string& value) {
  if (!value.compare("true") || 
      !value.compare("on") || 
      !value.compare("yes") ||
      !value.compare("1"))
    return true;
  else
    return false;
}