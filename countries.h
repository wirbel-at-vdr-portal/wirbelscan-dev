/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <string>

namespace COUNTRY {

enum channellist_t {
  ATSC_VSB                = 1,
  ATSC_QAM                = 2,
  DVBT_AU                 = 3,
  DVBT_DE                 = 4,
  DVBT_FR                 = 5,
  DVBT_GB                 = 6,
  DVBC_QAM                = 7,
  DVBC_FI                 = 8,
  DVBC_FR                 = 9,
  DVBC_BR                 = 10,
  ISDBT_6MHZ              = 11,
  USERLIST                = 999
};

typedef struct cCountry {
  const char* short_name;
  size_t      id;
  const char* full_name;
} _country;

extern struct cCountry country_list[];

size_t country_count(void);
void print_countries(void);
int choose_country(std::string country,
                   int& atsc,
                   int& dvb_cable,
                   uint16_t& scan_type,
                   int& channellist);

int txt_to_country(std::string id);
std::string country_to_short_name(size_t idx);
std::string country_to_full_name(size_t idx);


int base_offset(int channel, int channellist);
int freq_step  (int channel, int channellist);
int bandwidth  (int channel, int channellist);
int freq_offset(int channel, int channellist, int index);
int max_dvbc_srate(int bandwidth);

int dvbt_transmission_mode(int channel, int channellist);

int dvbc_qam_max(int channel, int channellist);
int dvbc_qam_min(int channel, int channellist);

int atsc_is_vsb(int atsc);
int atsc_is_qam(int atsc);

int get_user_country(void);

} //end of namespace
