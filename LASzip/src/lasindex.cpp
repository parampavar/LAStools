/*
===============================================================================

  FILE:  lasindex.cpp

  CONTENTS:

    see corresponding header file

  PROGRAMMERS:

    info@rapidlasso.de  -  https://rapidlasso.de

  COPYRIGHT:

    (c) 2007-2022, rapidlasso GmbH - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the Apache Public License 2.0 published by the Apache Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  CHANGE HISTORY:

    see corresponding header file

===============================================================================
*/
#include "lasindex.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

#include "lasquadtree.hpp"
#include "lasinterval.hpp"
#ifdef LASZIPDLL_EXPORTS
#include "lasreadpoint.hpp"
#else
#include "lasreader.hpp"
#endif
#include "bytestreamin_file.hpp"
#include "bytestreamout_file.hpp"
#include "lasmessage.hpp"
#include "mydefs.hpp"

typedef std::unordered_map<I32, U32> my_cell_hash;

LASindex::LASindex()
{
  spatial = 0;
  interval = 0;
  have_interval = FALSE;
  start = 0;
  end = 0;
  full = 0;
  total = 0;
  cells = 0;
}

LASindex::~LASindex()
{
  if (spatial) delete spatial;
  if (interval) delete interval;
}

void LASindex::prepare(LASquadtree* spatial, I32 threshold)
{
  if (this->spatial) delete this->spatial;
  this->spatial = spatial;
  if (this->interval) delete this->interval;
  this->interval = new LASinterval(threshold);
}

BOOL LASindex::add(const F64 x, const F64 y, const U32 p_index)
{
  I32 cell = spatial->get_cell_index(x, y);
  return interval->add(p_index, cell);
}

void LASindex::complete(U32 minimum_points, I32 maximum_intervals)
{
  LASMessage(LAS_VERBOSE, "before complete %d %d", minimum_points, maximum_intervals);
  if (get_message_log_level() <= LAS_VERBOSE)
    print();
  if (minimum_points)
  {
    I32 hash1 = 0;
    my_cell_hash cell_hash[2];
    // insert all cells into hash1
    interval->get_cells();
    while (interval->has_cells())
    {
      cell_hash[hash1][interval->index] = interval->full;
    }
    while (cell_hash[hash1].size())
    {
      I32 hash2 = (hash1+1)%2;
      cell_hash[hash2].clear();
      // coarsen if a coarser cell will still have fewer than minimum_points (and points in all subcells)
      BOOL coarsened = FALSE;
      U32 i, full;
      I32 coarser_index;
      U32 num_indices;
      U32 num_filled;
      I32* indices;
      my_cell_hash::iterator hash_element_inner;
      my_cell_hash::iterator hash_element_outer = cell_hash[hash1].begin();
      while (hash_element_outer != cell_hash[hash1].end())
      {
        if ((*hash_element_outer).second)
        {
          if (spatial->coarsen((*hash_element_outer).first, &coarser_index, &num_indices, &indices))
          {
            full = 0;
            num_filled = 0;
            for (i = 0; i < num_indices; i++)
            {
              if ((*hash_element_outer).first == indices[i])
              {
                hash_element_inner = hash_element_outer;
              }
              else
              {
                hash_element_inner = cell_hash[hash1].find(indices[i]);
              }
              if (hash_element_inner != cell_hash[hash1].end())
              {
                full += (*hash_element_inner).second;
                (*hash_element_inner).second = 0;
                num_filled++;
              }
            }
            if ((full < minimum_points) && (num_filled == num_indices))
            {
              interval->merge_cells(num_indices, indices, coarser_index);
              coarsened = TRUE;
              cell_hash[hash2][coarser_index] = full;
            }
          }
        }
        hash_element_outer++;
      }
      if (!coarsened) break;
      hash1 = (hash1+1)%2;
    }
    // tell spatial about the existing cells
    interval->get_cells();
    while (interval->has_cells())
    {
      spatial->manage_cell(interval->index);
    }
    LASMessage(LAS_VERBOSE, "after minimum_points %d", minimum_points);
    if (get_message_log_level() <= LAS_VERBOSE)
      print();
  }
  if (maximum_intervals < 0)
  {
    maximum_intervals = -maximum_intervals*interval->get_number_cells();
  }
  if (maximum_intervals)
  {
    interval->merge_intervals(maximum_intervals);
    LASMessage(LAS_VERBOSE, "after maximum_intervals %d", maximum_intervals);
    if (get_message_log_level() <= LAS_VERBOSE)
      print();
  }
}

void LASindex::print()
{
  U32 total_cells = 0;
  U32 total_full = 0;
  U32 total_total = 0;
  U32 total_intervals = 0;
  U32 total_check;
  U32 intervals;
  interval->get_cells();
  while (interval->has_cells())
  {
    total_check = 0;
    intervals = 0;
    while (interval->has_intervals())
    {
      total_check += interval->end-interval->start+1;
      intervals++;
    }
    if (total_check != interval->total)
    {
      LASMessage(LAS_VERBOSE, "total_check %d != interval->total %d", total_check, interval->total);
    }
    LASMessage(LAS_VERY_VERBOSE, "cell %d intervals %d full %d total %d (%.2f)", interval->index, intervals, interval->full, interval->total, 100.0f*interval->full/interval->total);
    total_cells++;
    total_full += interval->full;
    total_total += interval->total;
    total_intervals += intervals;
  }
  LASMessage(LAS_VERY_VERBOSE, "total cells/intervals %d/%d full %d (%.2f)", total_cells, total_intervals, total_full, 100.0f*total_full/total_total);
}

LASquadtree* LASindex::get_spatial() const
{
  return spatial;
}

LASinterval* LASindex::get_interval() const
{
  return interval;
}

BOOL LASindex::intersect_rectangle(const F64 r_min_x, const F64 r_min_y, const F64 r_max_x, const F64 r_max_y)
{
  have_interval = FALSE;
  cells = spatial->intersect_rectangle(r_min_x, r_min_y, r_max_x, r_max_y);
//  LASMessage(LAS_VERBOSE, "%d cells of %g/%g %g/%g intersect rect %g/%g %g/%g", num_cells, spatial->get_min_x(), spatial->get_min_y(), spatial->get_max_x(), spatial->get_max_y(), r_min_x, r_min_y, r_max_x, r_max_y);
  if (cells)
    return merge_intervals();
  return FALSE;
}

BOOL LASindex::intersect_tile(const F32 ll_x, const F32 ll_y, const F32 size)
{
  have_interval = FALSE;
  cells = spatial->intersect_tile(ll_x, ll_y, size);
//  LASMessage(LAS_VERBOSE, "%d cells of %g/%g %g/%g intersect tile %g/%g/%g", num_cells, spatial->get_min_x(), spatial->get_min_y(), spatial->get_max_x(), spatial->get_max_y(), ll_x, ll_y, size);
  if (cells)
    return merge_intervals();
  return FALSE;
}

BOOL LASindex::intersect_circle(const F64 center_x, const F64 center_y, const F64 radius)
{
  have_interval = FALSE;
  cells = spatial->intersect_circle(center_x, center_y, radius);
//  LASMessage(LAS_VERBOSE, "%d cells of %g/%g %g/%g intersect circle %g/%g/%g", num_cells, spatial->get_min_x(), spatial->get_min_y(), spatial->get_max_x(), spatial->get_max_y(), center_x, center_y, radius);
  if (cells)
    return merge_intervals();
  return FALSE;
}

BOOL LASindex::get_intervals()
{
  have_interval = FALSE;
  return interval->get_merged_cell();
}

BOOL LASindex::has_intervals()
{
  if (interval->has_intervals())
  {
    start = interval->start;
    end = interval->end;
    full = interval->full;
    have_interval = TRUE;
    return TRUE;
  }
  have_interval = FALSE;
  return FALSE;
}

BOOL LASindex::read(FILE* file)
{
  if (file == 0) return FALSE;
  ByteStreamIn* stream;
  if (IS_LITTLE_ENDIAN())
    stream = new ByteStreamInFileLE(file);
  else
    stream = new ByteStreamInFileBE(file);
  if (!read(stream))
  {
    delete stream;
    return FALSE;
  }
  delete stream;
  return TRUE;
}

BOOL LASindex::write(FILE* file) const
{
  if (file == 0) return FALSE;
  ByteStreamOut* stream;
  if (IS_LITTLE_ENDIAN())
    stream = new ByteStreamOutFileLE(file);
  else
    stream = new ByteStreamOutFileBE(file);
  if (!write(stream))
  {
    delete stream;
    return FALSE;
  }
  delete stream;
  return TRUE;
}

BOOL LASindex::read(const char* file_name)
{
  FILE* file;
  std::string fn = std::string(file_name);
  if (fn.empty()) return FALSE;
  if (fn.length() <= 4) {
    laserror("lasindex: file name invalid: '%s'", fn.c_str());
  }
  fn = FileExtSet(fn, ".lax");
  file = LASfopen(fn.c_str(), "rb");
  if (file == 0) {
    return FALSE;
  } 
  else if (!read(file)) {
    laserror("(LASindex): cannot read '%s'", fn);
    fclose(file);
    return FALSE;
  } else {
    fclose(file);
    return TRUE;
  }
}

BOOL LASindex::append(const char* file_name) const
{
#ifdef LASZIPDLL_EXPORTS
  return FALSE;
#else
  LASreadOpener lasreadopener;

  if (file_name == 0) return FALSE;

  // open reader

  LASreader* lasreader = lasreadopener.open(file_name);
  if (lasreader == 0) return FALSE;
  if (lasreader->header.laszip == 0) return FALSE;

  // close reader

  lasreader->close();

  FILE* file = LASfopen(file_name, "rb");

  ByteStreamIn* bytestreamin = 0;
  if (IS_LITTLE_ENDIAN())
    bytestreamin = new ByteStreamInFileLE(file);
  else
    bytestreamin = new ByteStreamInFileBE(file);

  // maybe write LASindex EVLR start position into LASzip VLR

  I64 offset_laz_vlr = -1;

  // where to write LASindex EVLR that will contain the LAX file

  I64 number_of_special_evlrs = lasreader->header.laszip->number_of_special_evlrs;
  I64 offset_to_special_evlrs = lasreader->header.laszip->offset_to_special_evlrs;

  if ((number_of_special_evlrs == -1) && (offset_to_special_evlrs == -1))
  {
    bytestreamin->seekEnd();
    number_of_special_evlrs = 1;
    offset_to_special_evlrs = bytestreamin->tell();

    // find LASzip VLR

    I64 total = lasreader->header.header_size + 2;
    U32 number_of_variable_length_records = lasreader->header.number_of_variable_length_records + 1 + (lasreader->header.vlr_lastiling != 0) + (lasreader->header.vlr_lasoriginal != 0);

    for (U32 u = 0; u < number_of_variable_length_records; u++)
    {
      bytestreamin->seek(total);

      CHAR user_id[LAS_VLR_USER_ID_CHAR_LEN];
      try { bytestreamin->getBytes((U8*)user_id, 16); } catch(...)
      {
        laserror("reading header.vlrs[%d].user_id", u);
        return FALSE;
      }
      if (strcmp(user_id, "laszip encoded") == 0)
      {
        offset_laz_vlr = bytestreamin->tell() - 18;
        break;
      }
      U16 record_id;
      try { bytestreamin->get16bitsLE((U8*)&record_id); } catch(...)
      {
        laserror("reading header.vlrs[%d].record_id", u);
        return FALSE;
      }
      U16 record_length_after_header;
      try { bytestreamin->get16bitsLE((U8*)&record_length_after_header); } catch(...)
      {
        laserror("reading header.vlrs[%d].record_length_after_header", u);
        return FALSE;
      }
      total += (54 + record_length_after_header);
    }

    if (number_of_special_evlrs == -1) return FALSE;
  }

  delete bytestreamin;
  fclose(file);

  ByteStreamOut* bytestreamout;
  file = LASfopen(file_name, "rb+");

  if (IS_LITTLE_ENDIAN())
    bytestreamout = new ByteStreamOutFileLE(file);
  else
    bytestreamout = new ByteStreamOutFileBE(file);
  bytestreamout->seek(offset_to_special_evlrs);

  LASevlr lax_evlr;
  snprintf(lax_evlr.user_id, sizeof(lax_evlr.user_id), "LAStools");
  lax_evlr.record_id = 30;
  snprintf(lax_evlr.description, sizeof(lax_evlr.description), "LAX spatial indexing (LASindex)");

  bytestreamout->put16bitsLE((const U8*)&(lax_evlr.reserved));
  bytestreamout->putBytes((const U8*)lax_evlr.user_id, 16);
  bytestreamout->put16bitsLE((const U8*)&(lax_evlr.record_id));
  bytestreamout->put64bitsLE((const U8*)&(lax_evlr.record_length_after_header));
  bytestreamout->putBytes((const U8*)lax_evlr.description, 32);

  if (!write(bytestreamout))
  {
    laserror("(LASindex): cannot append LAX to '%s'", file_name);
    delete bytestreamout;
    fclose(file);
    delete lasreader;
    return FALSE;
  }

  // update LASindex EVLR

  lax_evlr.record_length_after_header = bytestreamout->tell() - offset_to_special_evlrs - 60;
  bytestreamout->seek(offset_to_special_evlrs + 20);
  bytestreamout->put64bitsLE((const U8*)&(lax_evlr.record_length_after_header));

  // maybe update LASzip VLR

  if (number_of_special_evlrs != -1)
  {
    bytestreamout->seek(offset_laz_vlr + 54 + 16);
    bytestreamout->put64bitsLE((const U8*)&number_of_special_evlrs);
    bytestreamout->put64bitsLE((const U8*)&offset_to_special_evlrs);
  }

  // close writer

  bytestreamout->seekEnd();
  delete bytestreamout;
  fclose(file);

  // delete reader

  delete lasreader;

  return TRUE;
#endif
}

BOOL LASindex::write(const char* file_name) const
{
  FILE* file;
  std::string fn = std::string(file_name);
  if (fn.empty()) return FALSE;
  if (fn.length() <= 4) {
    laserror("lasindex: file name invalid: '%s'", fn.c_str());
  }
  fn = FileExtSet(fn, ".lax");
  file = LASfopen(fn.c_str(), "wb");
  if (file == 0) {
    laserror("(LASindex): cannot open file '%s' for write", fn.c_str());
    return FALSE;
  } else if (!write(file)) {
    laserror("(LASindex): cannot write '%s'", fn);
    fclose(file);
    return FALSE;
  } else {
    fclose(file);
    return TRUE;
  }
}

BOOL LASindex::read(ByteStreamIn* stream)
{
  if (spatial)
  {
    delete spatial;
    spatial = 0;
  }
  if (interval)
  {
    delete interval;
    interval = 0;
  }
  char signature[4];
  try { stream->getBytes((U8*)signature, 4); } catch (...)
  {
    laserror("(LASindex): reading signature");
    return FALSE;
  }
  if (strncmp(signature, "LASX", 4) != 0)
  {
    laserror("(LASindex): wrong signature %4s instead of 'LASX'", signature);
    return FALSE;
  }
  U32 version;
  try { stream->get32bitsLE((U8*)&version); } catch (...)
  {
    laserror("(LASindex): reading version");
    return FALSE;
  }
  // read spatial quadtree
  spatial = new LASquadtree();
  if (!spatial->read(stream))
  {
    laserror("(LASindex): cannot read LASspatial (LASquadtree)");
    return FALSE;
  }
  // read interval
  interval = new LASinterval();
  if (!interval->read(stream))
  {
    laserror("(LASindex): reading LASinterval");
    return FALSE;
  }
  // tell spatial about the existing cells
  interval->get_cells();
  while (interval->has_cells())
  {
    spatial->manage_cell(interval->index);
  }
  return TRUE;
}

BOOL LASindex::write(ByteStreamOut* stream) const
{
  if (!stream->putBytes((const U8*)"LASX", 4))
  {
    laserror("(LASindex): writing signature");
    return FALSE;
  }
  U32 version = 0;
  if (!stream->put32bitsLE((const U8*)&version))
  {
    laserror("(LASindex): writing version");
    return FALSE;
  }
  // write spatial quadtree
  if (!spatial->write(stream))
  {
    laserror("(LASindex): cannot write LASspatial (LASquadtree)");
    return FALSE;
  }
  // write interval
  if (!interval->write(stream))
  {
    laserror("(LASindex): writing LASinterval");
    return FALSE;
  }
  return TRUE;
}

// seek to next interval point

#ifdef LASZIPDLL_EXPORTS
BOOL LASindex::seek_next(LASreadPoint* reader, I64 &p_count)
{
  if (!have_interval)
  {
    if (!has_intervals()) return FALSE;
    reader->seek((U32)p_count, start);
    p_count = start;
  }
  if (p_count == end)
  {
    have_interval = FALSE;
  }
  return TRUE;
}
#else
BOOL LASindex::seek_next(LASreader* lasreader)
{
  if (!have_interval)
  {
    if (!has_intervals()) return FALSE;
    lasreader->seek(start);
  }
  if (lasreader->p_idx == end)
  {
    have_interval = FALSE;
  }
  return TRUE;
}
#endif

// merge the intervals of non-empty cells
BOOL LASindex::merge_intervals()
{
  if (spatial->get_intersected_cells())
  {
    U32 used_cells = 0;
    while (spatial->has_more_cells())
    {
      if (interval->get_cell(spatial->current_cell))
      {
        interval->add_current_cell_to_merge_cell_set();
        used_cells++;
      }
    }
//    LASMessage(LAS_VERBOSE, "(LASindex): used %d cells of total %d", used_cells, interval->get_number_cells());
    if (used_cells)
    {
      BOOL r = interval->merge();
      full = interval->full;
      total = interval->total;
      interval->clear_merge_cell_set();
      return r;
    }
  }
  return FALSE;
}
