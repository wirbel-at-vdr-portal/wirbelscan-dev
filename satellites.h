/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <string>
#include <cstdint> // uint{8,16,32}_t


int choose_satellite(std::string satellite, int& channellist);
int txt_to_satellite(std::string id);
size_t sat_count(void);
std::string satellite_to_short_name(size_t idx);
std::string satellite_to_full_name(size_t idx);
int rotor_position_to_sat_list_index(int rotor_position);
void print_satellites(void);


/******************************************************************************
 * only used for storage of data
 *****************************************************************************/
struct __sat_transponder {
  uint8_t                         modulation_system;
  uint32_t                        intermediate_frequency;
  uint8_t                         polarization;
  uint32_t                        symbol_rate;
  uint8_t                         fec_inner;
  uint8_t                         rolloff;
  uint8_t                         modulation_type;
  uint8_t                         stream_id;
};

struct cSat {
  const char*                     short_name;
  const size_t                    id;
  const char*                     full_name;
  const struct __sat_transponder* items;
  const size_t                    item_count;
  const uint8_t                   west_east_flag;
  const uint16_t                  orbital_position;
  int                             rotor_position;     // Note: *not* const
  const char*                     source_id;          // VDR sources.conf
};

extern struct cSat sat_list[];

#define SAT_COUNT(x)             (sizeof(x)/sizeof(struct cSat))
#define SAT_TRANSPONDER_COUNT(x) (sizeof(x)/sizeof(struct __sat_transponder))
