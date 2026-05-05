/*
===============================================================================

  FILE:  validate_writer.hpp
  
  CONTENTS:
  
    Writes a the validation report in a very simple txt, xml or json.

  PROGRAMMERS:
  
    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
  
  COPYRIGHT:
  
    (c) 2013, martin isenburg, rapidlasso - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    5 February 2026 -- created

===============================================================================
*/

#ifndef VALIDATE_WRITER_HPP
#define VALIDATE_WRITER_HPP

#include "mydefs.hpp"

#include <stdio.h>
#include <string>
#include <sstream>

class ValidateWriter {
 public:
  enum class ContainerType { 
      Object, 
      Array 
  };

  virtual BOOL is_open() const = 0;
  virtual BOOL open(const std::string& key) = 0;
  virtual BOOL begin(const std::string& key, ContainerType type = ContainerType::Object) = 0;
  virtual BOOL beginsub(const std::string& key, ContainerType type = ContainerType::Object) = 0;
  virtual BOOL write(I32 value) = 0;
  virtual BOOL write(const std::string& value) = 0;
  virtual BOOL write(const std::string& key, I32 value) = 0;
  virtual BOOL write(const std::string& key, const std::string& value) = 0;
  virtual BOOL write(const std::string& variable, const std::string& key, const std::string& note) = 0;
  virtual BOOL write_final() = 0;
  virtual BOOL endsub(const std::string& key) = 0;
  virtual BOOL end(const std::string& key) = 0;
  virtual void next_file() = 0;

  ValidateWriter(FILE* file);
  virtual ~ValidateWriter() = default;

 protected:
  int indent;
  BOOL sub;
  FILE* file;

  void printIndent(std::ostringstream &stream);
};

#endif