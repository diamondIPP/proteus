#include "exception.h"

using namespace std;

Exception::Exception(const std::string &error) :
  _str(error)
{}

Exception::Exception(const std::string &error, int line) :
  _str(error)
{
  std::stringstream ss;
  ss << line;
  _str += ss.str();
}

const char* Exception::what() const throw() {
  return _str.c_str();
}
      
