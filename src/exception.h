#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

/** @class Exception
 **    
 ** @brief A class to handle thrown exceptions.
 ** @author Sergio.Gonzalez.Sevilla@cern.h
 */
class Exception : public std::exception {
 public:
  /** Constructor */
  Exception(const std::string &error);

  /** Constructor */
  Exception(const std::string &error, int line);

  /** Destructor */
  virtual ~Exception() throw() {};

  /** what function, typically  to be catch*/
  const char* what() const throw();

 private:
  std::string _str; 

}; // end of class

#endif // EXCEPTION_H
