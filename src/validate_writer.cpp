/*
===============================================================================

  FILE:  validate_writer.cpp
  
  CONTENTS:
  
    see corresponding header file
  
  PROGRAMMERS:
  
    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
  
  COPYRIGHT:
  
    (c) 2026, martin isenburg, rapidlasso - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see corresponding header file
  
===============================================================================
*/

#include "validate_writer.hpp"

ValidateWriter::ValidateWriter(FILE* file) {
  indent = 0;
  sub = FALSE;
  this->file = file;
}

void ValidateWriter::printIndent(std::ostringstream& stream) {
  if (file == nullptr) return;

  for (int i = 0; i < indent; ++i) {
    stream << "  ";  // 2 spaces per level
  }
}