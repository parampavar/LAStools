/*
===============================================================================

  FILE:  validate_csv_writer.hpp

  CONTENTS:

    Writes a LAScheck simplified report to a file in a very simple CSV format.

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

    24 February 2026 -- created

===============================================================================
*/
#ifndef CSV_VALIDATION_WRITER_HPP
#define CSV_VALIDATION_WRITER_HPP

#include "json.hpp"
#include "mydefs.hpp"
#include "validate_writer.hpp"

#include <stdio.h>
#include <vector>

class ValidateCsvWriter : public ValidateWriter {
 public:
  using ValidateWriter::ValidateWriter;

  BOOL is_open() const override;
  BOOL open(const std::string& key) override;
  BOOL begin(const std::string& key, ContainerType type = ContainerType::Object) override;
  BOOL beginsub(const std::string& key, ContainerType type = ContainerType::Object) override;
  BOOL write(I32 value) override;
  BOOL write(const std::string& value) override;
  BOOL write(const std::string& key, I32 value) override;
  BOOL write(const std::string& key, const std::string& value) override;
  BOOL write(const std::string& variable, const std::string& key, const std::string& note) override;
  BOOL write_final() override;
  BOOL endsub(const std::string& key) override;
  BOOL end(const std::string& key) override;

  void next_file() override;

  ~ValidateCsvWriter() override = default;

 private:
  std::unordered_map<std::string, std::vector<std::string>> csv_data;
  std::vector<std::string> key_order;
  std::map<std::string, std::string> detail_data;
  size_t current_row = 0;
  BOOL last_value_int = FALSE;

  void flush_detail_data();
};

#endif
