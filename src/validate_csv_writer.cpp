/*
===============================================================================

  FILE:  validate_csv_writer.cpp

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

#include "validate_csv_writer.hpp"
#include <sstream>

/// Write all key-value pairs in detail_data to the csv structure
void ValidateCsvWriter::flush_detail_data() {
  if (detail_data.empty()) return;

  for (const auto& [key, value] : detail_data) {
    write(key, value);
  }

  detail_data.clear();
}

/// Increments the counter for the notes when a report is generated with multiple input files
void ValidateCsvWriter::next_file() {
  flush_detail_data();
  current_row++;
}

BOOL ValidateCsvWriter::is_open() const {
  return (BOOL)(file != nullptr);
}

/// Initializes the writer and opens the root element/container identified by the given key
BOOL ValidateCsvWriter::open(const std::string& key) {
  if (file == nullptr) return FALSE;

  return TRUE;
}

/// Begins a new structured section identified by the given key and makes it the current write context
BOOL ValidateCsvWriter::begin(const std::string& key, ContainerType type) {
  if (file == nullptr) return FALSE;

  return TRUE;
}

/// Begins a sub-section within the current section and makes it the active write context
BOOL ValidateCsvWriter::beginsub(const std::string& key, ContainerType type) {
  return begin(key);
}

/// Writes a simple value into the current context
BOOL ValidateCsvWriter::write(const std::string& value) {
  if (file == nullptr) return FALSE;

  return TRUE;
}

/// Writes a numeric value into the current context
BOOL ValidateCsvWriter::write(I32 value) {
  if (file == nullptr) return FALSE;

  return TRUE;
}

/// Writes a key-value pair into the current context
BOOL ValidateCsvWriter::write(const std::string& key, const std::string& value) {
  if (file == nullptr) return FALSE;

  // keep key order
  if (csv_data.find(key) == csv_data.end()) {
    key_order.push_back(key);
  }

  std::vector<std::string>& column = csv_data[key];

  if (column.size() <= current_row) column.resize(current_row + 1);

  std::string escaped_value = escape_csv_value(value);

  if (last_value_int == TRUE) {
    column[current_row] = escaped_value;  // no quotes
    last_value_int = FALSE;
  } else {
    column[current_row] = "\"" + escaped_value + "\"";  // strings always quote
  }

  return TRUE;
}

/// Writes a numeric key-value pair into the current context
BOOL ValidateCsvWriter::write(const std::string& key, I32 value) {
  if (file == nullptr) return FALSE;
  last_value_int = TRUE;

  return write(key, std::to_string(value));
}

/// Writes a structured entry consisting of a variable identifier and optional descriptive note under the given key
BOOL ValidateCsvWriter::write(const std::string& variable, const std::string& key, const std::string& note) {
  if (file == nullptr) return FALSE;

  std::string entry = variable;

  if (!note.empty()) entry += ": " + note;

  auto detail = detail_data.find(key);

  if (detail == detail_data.end()) {
    // create key new
    detail_data[key] = entry;
  } else {
    // extend key
    detail->second += ", " + entry;
  }

  return TRUE;
}

/// Finally, write the entire report to the output
BOOL ValidateCsvWriter::write_final() {
  if (file == nullptr) return FALSE;

  flush_detail_data();

  std::ostringstream stream;

  // 1. Write header
  bool first = true;
  for (const auto& key : key_order) {
    if (!first) stream << ";";
    stream << key;
    first = false;
  }
  stream << "\n";

  // 2. Write data rows
  for (size_t row = 0; row <= current_row; ++row) {
    first = true;

    for (const auto& key : key_order) {
      if (!first) stream << ";";

      const auto& column = csv_data[key];
      if (row < column.size()) {
        stream << column[row];
      }

      first = false;
    }
    stream << "\n";
  }

  std::string out = stream.str();
  fwrite(out.data(), 1, out.size(), file);

  return TRUE;
}

/// Closes the current sub-section and restores the previous write context
BOOL ValidateCsvWriter::endsub(const std::string& key) {
  if (file == nullptr) return FALSE;

  return TRUE;
}

/// Closes the current section and restores the parent context; finalizes output if the root section is closed
BOOL ValidateCsvWriter::end(const std::string& key) {
  if (file == nullptr) return FALSE;

   return TRUE;
}