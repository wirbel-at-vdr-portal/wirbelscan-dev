/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#define SAT_COUNT(x)             (sizeof(x)/sizeof(struct cSat))
#define SAT_TRANSPONDER_COUNT(x) (sizeof(x)/sizeof(struct __sat_transponder))
#define B(ID) static const struct __sat_transponder ID[] = {
#define E(ID) };
#include <string>
#include "common.h"
#include "satellites.h"
#include "satellites.dat"


/******************************************************************************
 * convert position constant to index number
 *****************************************************************************/
int txt_to_satellite(std::string id) {
  for(size_t i=0; i<SAT_COUNT(sat_list); i++)
     if (id == sat_list[i].short_name)
        return sat_list[i].id;
  return -1;
}


/******************************************************************************
 * return numbers of satellites defined.
 *****************************************************************************/
size_t sat_count(void) {
  return SAT_COUNT(sat_list);
}


/******************************************************************************
 * convert index number to position constant
 *****************************************************************************/
std::string satellite_to_short_name(size_t idx) {
  for(size_t i=0; i<SAT_COUNT(sat_list); i++)
     if (idx == sat_list[i].id)
        return sat_list[i].short_name;
  return "??";
}


/******************************************************************************
 * convert index number to satellite name
 *****************************************************************************/
std::string satellite_to_full_name(size_t idx) {
  for(size_t i=0; i<SAT_COUNT(sat_list); i++)
     if (idx == sat_list[i].id)
        return sat_list[i].full_name;
  warning("SATELLITE CODE NOT DEFINED. PLEASE RE-CHECK WETHER YOU TYPED CORRECTLY.\n");
  mSleep(5000);
  return "??";
}


/******************************************************************************
 * return index number from rotor position
 *****************************************************************************/
int rotor_position_to_sat_list_index(int rotor_position) {
  for(size_t i=0; i<SAT_COUNT(sat_list); i++)
     if (rotor_position == sat_list[i].rotor_position)
        return i;
  return 0;
}


/******************************************************************************
 * print list of all satellites
 *****************************************************************************/
void print_satellites(void) {
  for(size_t i=0; i<SAT_COUNT(sat_list); i++)
     info("\t%s\t\t%s\n", sat_list[i].short_name, sat_list[i].full_name);
}


/******************************************************************************
 * choose a satellite by it's id.
 *****************************************************************************/
int choose_satellite(std::string satellite, int& channellist) {
  int retval = 0;
  channellist = txt_to_satellite(satellite);
  if (channellist < 0) {
     channellist = S19E2;
     warning("\n\nSATELLITE CODE IS NOT DEFINED. FALLING BACK TO \"S19E2\"\n\n");
     mSleep(10000);
     retval = -1;
     }
  info("using settings for %s\n", satellite_to_full_name(channellist).c_str());
  return retval;
}
