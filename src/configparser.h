#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <string>
#include <sstream>
#include <fstream>
#include <vector>

/** @class ConfigParser
 **    
 ** @brief A class to parse configuration files.
 **
 */
class ConfigParser{
  
 public:
  struct Row {
    std::string header;
    std::string key;
    std::string value;
    bool isHeader;
  };
  
 public:
  ConfigParser(const char* filePath, int prinLEvel=0);
  
  const Row* getRow(unsigned int n) const;
  unsigned int getNumRows() const { return _numRows; }
  std::vector<Row> getParsedConents() const { return _parsedContents; }
  const char* getFilePath() const { return _filePath; }
  
  static double valueToNumerical(const std::string& value);
  static bool   valueToLogical(const std::string& value);
  
  template <typename T>
    static void valueToVec(const std::string& value, std::vector<T>& vec){
    if (value.empty())
      return;
    std::stringstream stream;
    stream.str(value);
    std::string buffer;
    while (stream.good()) {
      getline(stream, buffer, ',');
      std::stringstream ss;
      ss.str(buffer);
      T num;
      ss >> num;
      vec.push_back(num);
    }
  }

  void print();
  
 private:
  ConfigParser(const ConfigParser&); // Disable the copy constructor
  ConfigParser& operator=(const ConfigParser&); // Disable the assignment operator

  bool checkRange(size_t start, size_t end);
  
  /** These functions return -1 if they can't find the desired element */
  int readNextLine(std::ifstream& input); //<! Fill line buffer with next line
  int parseForHeader(); //<! Parse the line buffer for a header
  int parseForKey(); //<! Parse the line buffer for a key
  int parseForLink(); //<! Parse the line for a link to another config
  void parseContents(std::ifstream& input); //<! Parse the entire file's contents
  
 private:
  const char*   _filePath;
  std::ifstream _inputFile;
  std::string   _lineBuffer;
  std::string   _currentHeader;
  std::string   _currentKey;
  std::string   _currentValue;
  
  unsigned int _numRows;
  std::vector<Row> _parsedContents;

  int _printLevel; 

}; // end of class

#endif // CONFIGPARSER_H
