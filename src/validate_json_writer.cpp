/*
===============================================================================

  FILE:  validate_json_writer.cpp

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
#include "validate_json_writer.hpp"
#include <unordered_set>

BOOL ValidateJsonWriter::is_open() const {
  return (BOOL)(file != nullptr);
}

/// Initializes the writer and opens the root element/container identified by the given key
BOOL ValidateJsonWriter::open(const std::string& key) {
  if (file == nullptr) return FALSE;

  root = Json::object();
  root[key] = Json::object();
  stack.clear();
  stack.push_back(&root[key]);
  return TRUE;
}

/// Begins a new structured section identified by the given key and makes it the current write context
BOOL ValidateJsonWriter::begin(const std::string& key, ContainerType type) {
  if (file == nullptr || stack.empty()) return FALSE;

  Json& current = *stack.back();

  if (type == ContainerType::Array) {
    if (!current.contains(key)) current[key] = Json::array();
    current[key].push_back(Json::object());
    stack.push_back(&current[key].back());
  } else {
    current[key] = Json::object();
    stack.push_back(&current[key]);
  }

  return TRUE;
}

/// Begins a sub-section within the current section and makes it the active write context
BOOL ValidateJsonWriter::beginsub(const std::string& key, ContainerType type) {
  return begin(key, type);
}

/// Writes a simple value into the current context
BOOL ValidateJsonWriter::write(const std::string& value) {
  if (file == nullptr || stack.empty()) return FALSE;

  Json& node = *stack.back();

  if (!node.is_object()) return FALSE;

  node["value"] = value;
  return TRUE;
}

/// Writes a numeric value into the current context
BOOL ValidateJsonWriter::write(I32 value) {
  if (file == nullptr || stack.empty()) return FALSE;

  Json& node = *stack.back();

  if (!node.is_object()) return FALSE;

  node["value"] = value;
  return TRUE;
}

/// Writes a key-value pair into the current context
BOOL ValidateJsonWriter::write(const std::string& key, const std::string& value) {
  if (file == nullptr || stack.empty()) return FALSE;

  Json& current = *stack.back();
  current[key] = value;
  return TRUE;
}

/// Writes a numeric key-value pair into the current context
BOOL ValidateJsonWriter::write(const std::string& key, I32 value) {
  if (file == nullptr || stack.empty()) return FALSE;

  Json& current = *stack.back();
  current[key] = value;
  return TRUE;
}

/// Writes a structured entry consisting of a variable identifier and optional descriptive note under the given key
BOOL ValidateJsonWriter::write(const std::string& variable, const std::string& key, const std::string& note) {
  if (file == nullptr || stack.empty()) return FALSE;

  Json& current = *stack.back();

  // 1. check/create array
  if (!current.contains(key) || !current[key].is_array()) {
    current[key] = Json::array();
  }

  // 2. prepare the object
  Json obj;
  obj["variable"] = variable;
  if (!note.empty()) obj["note"] = note;

  current[key].push_back(obj);

  return TRUE;
}

/// Finally, write the entire report to the output
BOOL ValidateJsonWriter::write_final() {
  if (file == nullptr) return FALSE;

  // if root is closed, then write to the file
  if (stack.empty()) {
    std::string json = root.dump(2);
    json.push_back('\n');

    fwrite(json.data(), 1, json.size(), file);
  }

  return TRUE;
}

/// Closes the current sub-section and restores the previous write context
BOOL ValidateJsonWriter::endsub(const std::string& key) {
  if (file == nullptr || stack.size() <= 1) return FALSE;

  stack.pop_back();
  return TRUE;
}

/// Closes the current section and restores the parent context; finalizes output if the root section is closed
BOOL ValidateJsonWriter::end(const std::string& key) {
  if (file == nullptr || stack.empty()) return FALSE;

  stack.pop_back();

  return TRUE;
}
