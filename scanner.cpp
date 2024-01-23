/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#include <string>
#include <array>
#include <algorithm>     // std::min()
#include <vdr/sources.h>
#include <vdr/device.h>
#include "scanner.h"
#include "menusetup.h"
#include "common.h"
#include "satellites.h"
#include "scanfilter.h"
#include "statemachine.h"
#include "countries.h"
#include "wirbelscan_services.h"
#if VDRVERSNUM < 20301
   #error "Your VDR version is too old - STOP."
#endif

using namespace COUNTRY;
extern const char* WIRBELSCAN_VERSION;
int initialTransponders;
cScanner* Scanner = nullptr;

static unsigned int chan_to_freq(int channel, int channellist) {
  if (channellist == 999)
     dlog(6, "channellist="   + IntToStr(channellist) +
             ", base_offset=" + IntToStr(base_offset(channel, channellist)) +
             ", channel="     + IntToStr(channel) +
             ", step="        + IntToStr(freq_step(channel, channellist)));

  if (base_offset(channel, channellist) != -1)  // -1 == invalid
     return (base_offset(channel, channellist) + channel * freq_step(channel, channellist));

  return 0;
}


static int device_is_preferred(TChannel* Channel, std::string name, bool secondGen) {
  int preferred = 1; // no preferrence

  /* add other good/bad cards here. */
  if (name == "VLSI VES1820") {
     /* bad working FF dvb-c card, known to have qam256 probs */
     preferred = 0; // not preferred
     }
  else if (name == "Sony CXD2820R") {
     /* PCTV 290e known to have qam256 probs. */
     /* PCTV 290e doesnt support newer DVB-T2 */
     preferred = 0; // not preferred
     }
  else if (secondGen) {
     /* wirbelscan preferres devices which are DVB-{S,C,T}2 */
     preferred = 2; // preferred
     }
  return preferred;
}


cDevice* DefaultDevice(TChannel* Channel) {
  std::string preferred = wSetup.preferred[dmap[*(Channel->Source.c_str())]];

  for(int i=0; i<cDevice::NumDevices(); i++) {
     auto dev = cDevice::GetDevice(i);
     if (DeviceName(dev) == preferred)
        return dev;
     }
  return nullptr;
}


cDevice* GetPreferredDevice(TChannel* Channel) {
  cDevice* dev = DefaultDevice(Channel);

  if (dev) return dev;

  int preferred = 0;
  int pref_device = -1;
  std::string name;
  TChannel ch2nd;
  bool gen2 = false;

  std::string s;
  Channel->PrintTransponder(s);
  dlog(6, "'" + Channel->Source + "' " + s);

  // skip ChannelID check in cChannel::Parse()
  // we just want to find a device here, nothing else.
  int nid = Channel->NID;
  int sid = Channel->SID;
  Channel->NID = 0x2000;
  Channel->SID = 0x2000;
  cChannel c;
  Channel->VdrChannel(c);
  Channel->NID = nid;
  Channel->SID = sid;

  dlog(4, "testing '" + std::string(*c.ToText()) + "'");

  for(int i=0; i<cDevice::NumDevices(); i++) {
     // next line should never fail.
     if (!(dev = cDevice::GetDevice(i)))
        continue;
     name = dev->DeviceName();
     if (!dev->ProvidesTransponder(&c)) {
        dlog(4, "device " + IntToStr(dev->CardIndex()) + " = " + name +
                " (not usable)");
        continue;
        }

     if (Channel->Source[0] == 'S' or Channel->Source[0] == 'T') {
        ch2nd = &c;
        ch2nd.DelSys = 1;
        ch2nd.NID = 0x2000;
        ch2nd.SID = 0x2000;
        ch2nd.VdrChannel(c);
        gen2 = dev->ProvidesTransponder(&c);
        }
     else
        gen2 = false;
     dlog(4, "device " + IntToStr(dev->CardIndex()) + " = " + name);
     if (device_is_preferred(Channel, name, gen2) >= preferred) {
        preferred   = device_is_preferred(Channel, name, gen2);
        pref_device = i;
        }
     switch(preferred) {
        case 0:
           dlog(4, "device known to have probs. usable anyway..");
           break;
        case 1:
           dlog(4, "device has no gen2 delsys support.");
           break;
        case 2:
           dlog(4, "device has gen2 delsys support.");
           return dev;
        default:;
        }
     }
  if (pref_device >= 0) {
     dev = cDevice::GetDevice(pref_device);
     return dev;
     }
  return nullptr;
}



/*******************************************************************************
 * class cScanner
 ******************************************************************************/

cScanner::cScanner(const char* Description, int Type) :
  shouldstop(false), single(false),
  status(0), initialTransponders(0), newTransponders(0), thisChannel(-1),
  type(Type), dev(nullptr), aChannel(nullptr), StateMachine(nullptr)
{
  user[0] = user[1] = user[2] = 0; 
  Start();
}

cScanner::~cScanner(void) {
  dlog(5, "destroying scanner");
  Scanner = nullptr;
}

void cScanner::SetShouldstop(bool On) {
  shouldstop = On;
  if (StateMachine)
     StateMachine->DoStop();
}

bool cScanner::ActionAllowed(void) {
  return Running() and not shouldstop;
}

bool cScanner::Active(void) {
  return Running();
}

void cScanner::Progress(void) {
  extern TChannels ScannedTransponders;
  extern TChannels NewTransponders;

  lProgress = 0.5 + (100.0 * (ThisChannel() + ScannedTransponders.Count()) / (NewTransponders.Count() + InitialTransponders()));

  if (!initialTransponders)
     lProgress = 0;

  if (lProgress > 100)
     lProgress = 100;

  if (MenuScanning) {
     MenuScanning->SetCounters(thisChannel + ScannedTransponders.Count(), NewTransponders.Count() + initialTransponders);
     MenuScanning->SetProgress(lProgress);
     }
}

cDvbDevice* cScanner::DvbDevice(void) {
  return GetDvbDevice(dev);
}

void cScanner::Action(void) {
  bool crAuto, modAuto, invAuto, bwAuto, hAuto, tmAuto, gAuto, t2Support, roAuto, s2Support, vsbSupport, qamSupport;
  bool useNit = true;
  bool isSatip = false;
  int f = 0;
  int mod_parm, modulation_min = 0, modulation_max = 1;
  int sr_parm, dvbc_symbolrate_min = 0, dvbc_symbolrate_max = 1;
  int sys_parm = 0, thisSystem = -1;
  int streamid_parm = 0;
  int channel, channel_min = 0, channel_max = 133;
  int offs, freq_offset_min = 0, freq_offset_max = 2;
  int this_channellist = DVBT_DE, this_bandwidth = 8, this_qam = 999, atsc = ATSC_VSB, dvb;
  int qam_no_auto = 0, this_atsc = 0;
  uint16_t frontend_type = SCAN_SATELLITE;
  std::string country   = country_to_short_name(wSetup.CountryIndex);
  std::string satellite = satellite_to_short_name(wSetup.SatIndex);
  std::string channelname, shortname;
  int caps_inversion = 0, caps_qam = 999, caps_hierarchy = 0;
  int caps_fec = 999, caps_guard_interval = 999, caps_transmission_mode = 999;
  int caps_s2 = 1;
  std::string s;

  resetLists();
  thisChannel = 0;
  initialTransponders = 0;
  dev = nullptr;
  status = 1;
  if (MenuScanning) MenuScanning->SetStatus(status);
  dlog(3, "wirbelscan version " + std::string(WIRBELSCAN_VERSION) +
          " @ VDR " + std::string(VDRVERSION));

  switch(type) {
     case SCAN_TRANSPONDER: {
        using namespace WIRBELSCAN_SERVICE;
        cUserTransponder* t = new cUserTransponder(&wSetup.user[0]);
        this_channellist = USERLIST;
        useNit = t->UseNit();

        // disable all loops
        modulation_min      = modulation_max      = 0;
        dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
        channel_min         = channel_max         = 1;
        freq_offset_min     = freq_offset_max     = 0;
        single = true;
        thisChannel = -1;

        aChannel = new TChannel;

        switch(t->Type()) {
            case SCAN_TERRESTRIAL:
               dvb = frontend_type    = SCAN_TERRESTRIAL;
               aChannel->Source       = "T";
               aChannel->Frequency    = t->Frequency();
               aChannel->Inversion    = t->Inversion();
               aChannel->Bandwidth    = t->Bandwidth();
               aChannel->FEC          = t->FecHP();
               aChannel->FEC_low      = t->FecLP();
               aChannel->Modulation   = t->Modulation();
               aChannel->DelSys       = t->System();
               aChannel->Transmission = t->Transmission();
               aChannel->Guard        = t->Guard();
               aChannel->Hierarchy    = t->Hierarchy();
               break;
            case SCAN_CABLE:
               dvb = frontend_type = SCAN_CABLE;
               aChannel->Source       = "C";
               aChannel->Frequency    = t->Frequency();
               aChannel->Modulation   = t->Modulation();
               aChannel->Symbolrate   = t->Symbolrate();
               aChannel->Inversion    = t->Inversion();
               aChannel->FEC          = FEC_NONE;
               aChannel->DelSys       = t->System();
               break;
            case SCAN_SATELLITE:
               dvb = frontend_type    = SCAN_SATELLITE;
               aChannel->Source       = sat_list[t->Id()].source_id;
               aChannel->West         = sat_list[t->Id()].west_east_flag == WEST_FLAG;
               aChannel->OrbitalPos   = aChannel->West ?
                                         BCDtoDecimal(0x3600) - BCDtoDecimal(sat_list[t->Id()].orbital_position) :
                                                                BCDtoDecimal(sat_list[t->Id()].orbital_position);
               aChannel->Frequency    = t->Frequency();
               aChannel->Symbolrate   = t->Symbolrate();
               aChannel->Polarization = t->Polarisation() == 0 ? 'H':
                                        t->Polarisation() == 1 ? 'V':
                                        t->Polarisation() == 2 ? 'L': 'R';
               aChannel->FEC          = t->FecHP();
               aChannel->Modulation   = t->Modulation();
               aChannel->DelSys       = t->System();
               aChannel->Rolloff      = t->Rolloff();
               break;
            case SCAN_TERRCABLE_ATSC:
               dvb = frontend_type = SCAN_TERRCABLE_ATSC;
               //fixme: vsb vs qam here
               aChannel->Source       = "A";
               aChannel->Frequency    = t->Frequency();
               aChannel->Symbolrate   = t->Symbolrate();
               aChannel->Inversion    = t->Inversion();
               aChannel->FEC          = FEC_NONE;
               aChannel->Bandwidth    = 6;
               break;
            default:
               dlog(0, "unsupported user transponder type.");
            }
        if ((dev = GetPreferredDevice(aChannel)) == nullptr) {
           dlog(0, "No device available - exiting!");
           if (MenuScanning) MenuScanning->SetStatus((status = 2));
           DeleteNullptr(aChannel);
           return;
           }

        if (std::string(dev->DeviceName()).find("SAT>IP") == std::string::npos) {
           PrintDvbApi(s);
           dlog(5, s);

           switch(t->Type()) {
               case SCAN_TERRESTRIAL:
                  if (! GetTerrCapabilities(dev, &crAuto, &modAuto, &invAuto, &bwAuto, &hAuto, &tmAuto, &gAuto, &t2Support))
                     dlog(0, "ERROR: Could not query capabilites.");
                  break;
               case SCAN_CABLE:
                  if (! GetCableCapabilities(dev, &modAuto, &invAuto))
                     dlog(0, "ERROR: Could not query capabilites.");
                  break;
               case SCAN_SATELLITE:
                  if (! GetSatCapabilities(dev, &crAuto, &modAuto, &roAuto, &s2Support))
                     dlog(0, "ERROR: Could not query capabilites.");
                  break;
               case SCAN_TERRCABLE_ATSC:
                  if (! GetAtscCapabilities(dev, &modAuto, &invAuto, &vsbSupport, &qamSupport))
                     dlog(0, "ERROR: Could not query capabilites.");
                  break;
               default:;
               }
           }
        else {
           crAuto    = 0;
           modAuto   = 0;
           invAuto   = 0;
           bwAuto    = 0;
           hAuto     = 0;
           tmAuto    = 0;
           gAuto     = 0;
           t2Support = 1;
           crAuto    = 0;
           modAuto   = 0;
           roAuto    = 0;
           s2Support = 1;
           }
        lDeviceName = dev->DeviceName();
        dlog(3, "frontend " + lDeviceName);     
        if (MenuScanning)
           MenuScanning->SetDeviceName(lDeviceName);
        break;
        }
     case SCAN_TERRESTRIAL: {
        dvb = type;
        frontend_type = type;
        choose_country(country, atsc, dvb, frontend_type, this_channellist);
        /* find a dvb-t2 capable device using *some* channel */
        aChannel = new TChannel;
        aChannel->Name         = "???";
        aChannel->Source       = "T";
        aChannel->Frequency    = 474;
        aChannel->Inversion    = 999;
        aChannel->Bandwidth    = 8;
        aChannel->FEC          = 23;
        aChannel->Modulation   = 256;
        aChannel->DelSys       = 1;
        aChannel->Transmission = 8;
        aChannel->Guard        = 999;
        aChannel->Hierarchy    = 0;
        if ((dev = GetPreferredDevice(aChannel)) == nullptr) {
           dlog(0, "No DVB-T2 device available - trying fallback to DVB-T");
           if (MenuScanning) MenuScanning->SetStatus((status = 3));
           aChannel->Modulation   = 64;
           aChannel->DelSys       = 0;
           if ((dev = GetPreferredDevice(aChannel)) == nullptr) {
              dlog(0, "No device available - exiting!");
              if (MenuScanning)
                 MenuScanning->SetStatus((status = 2));
              DeleteNullptr(aChannel);
              return;
              }
           }

        if (std::string(dev->DeviceName()).find("SAT>IP") == std::string::npos) {
           PrintDvbApi(s);
           dlog(5, s);
           if (! GetTerrCapabilities(dev, &crAuto, &modAuto, &invAuto, &bwAuto, &hAuto, &tmAuto, &gAuto, &t2Support))
              dlog(0, "ERROR: Could not query capabilites.");
           }
        else {
           crAuto    = 0;
           modAuto   = 0;
           invAuto   = 0;
           bwAuto    = 0;
           hAuto     = 0;
           tmAuto    = 0;
           gAuto     = 0;
           t2Support = 1;
           }
        lDeviceName = dev->DeviceName();
        dlog(3, "frontend " + lDeviceName);     
        if (MenuScanning)
           MenuScanning->SetDeviceName(lDeviceName);

        if (invAuto)
           caps_inversion = 999;
        else {
           dlog(5, "I999 not supported, trying I" + IntToStr(wSetup.DVBT_Inversion) + ".");
           caps_inversion = wSetup.DVBT_Inversion;
           }

        if (modAuto)
           caps_qam = 999;
        else {
           caps_qam = 64;
           dlog(5, "M999 not supported, trying M" + IntToStr(caps_qam) + ".");
           }
        if (tmAuto)
           caps_transmission_mode = 999;
        else {
           const int t[] = {2,8,999,4,1,16,32};
           caps_transmission_mode = t[dvbt_transmission_mode(5, this_channellist)];
           dlog(5, "T999 not supported, trying T" + IntToStr(caps_transmission_mode) + ".");
           }
        if (gAuto)
           caps_guard_interval = 999;
        else {
           caps_guard_interval = 8;
           dlog(5, "G999 not supported, trying G" + IntToStr(caps_guard_interval) + ".");
           }
        if (hAuto)
           caps_hierarchy = 999;
        else {
          caps_hierarchy = 0;
          dlog(5, "Y999 not supported, trying Y" + IntToStr(caps_hierarchy) + ".");
           }
        if (crAuto)
           caps_fec = 999;
        else {
           caps_fec = 0;
           dlog(5, "C999 not supported, trying C" + IntToStr(caps_fec) + ".");
           }
        if (t2Support)
           dlog(5, "DVB-T2 supported");
        else
           dlog(0, "WARN: you are using an outdated DVB device: no DVB-T2 support.");

        // use mod as system T/T2 to avoid a further loop.
        // min = T2, max = T
        modulation_min = t2Support ? 0 : 1;
        modulation_max = 1;
        sys_parm = 0;
        // use srate as plp 0/1 to avoid a further loop.
        dvbc_symbolrate_min = 0;
        dvbc_symbolrate_max = t2Support ? 3 : 0;
        break;
        }
     case SCAN_CABLE: {
        dvb = type;
        frontend_type = type;
        choose_country(country, atsc, dvb, frontend_type, this_channellist);

        /* find a dvb-c capable device using *some* channel */
        aChannel = new TChannel;
        aChannel->Name       = "???";
        aChannel->Source     = "C";        
        aChannel->Frequency  = 410;
        aChannel->Modulation = 64;
        aChannel->Symbolrate = 6900;
        aChannel->Inversion  = 999;
        aChannel->FEC        = 0;
        aChannel->DelSys     = 0;
        if ((dev = GetPreferredDevice(aChannel)) == nullptr) {
           dlog(0, "No device available - exiting!");
           if (MenuScanning) MenuScanning->SetStatus((status = 2));
           DeleteNullptr(aChannel);
           return;
           }

        if (std::string(dev->DeviceName()).find("SAT>IP") == std::string::npos) {
           PrintDvbApi(s);
           dlog(5, s);
           if (! GetCableCapabilities(dev, &modAuto, &invAuto))
              dlog(0, "ERROR: Could not query capabilites.");
           }
        else {
           invAuto   = 0;
           modAuto   = 0;
           }  
        lDeviceName = dev->DeviceName();
        dlog(3, "frontend " + lDeviceName);
        if (MenuScanning)
           MenuScanning->SetDeviceName(lDeviceName);

        if (invAuto)
           caps_inversion = 999;
        else {
           caps_inversion = wSetup.DVBC_Inversion;
           dlog(5, "I999 not supported, trying I" + IntToStr(caps_inversion) + ".");
           }

        if (modAuto)
           caps_qam = 999;
        else {
           std::string s;
           for(int i = modulation_min; i <= modulation_max; i++) {
              if (s.size()) s += ", ";
              s += "M" + IntToStr(dvbc_modulation(i));
              }
           dlog(5, "M999 not supported, trying " + s + ".");
           caps_qam = 64;
           qam_no_auto = 1;
           }
        caps_fec = 0;
        switch(wSetup.DVBC_Symbolrate) {
           case 0: // auto
              dvbc_symbolrate_min = 0;
              dvbc_symbolrate_max = 1;
              break;
           case 1 ... 15:
              dvbc_symbolrate_min = dvbc_symbolrate_max = wSetup.DVBC_Symbolrate - 1;
              break;
           default:// all
              dvbc_symbolrate_min = 0;
              dvbc_symbolrate_max = 14;
              break;
           }
        break;
        }
     case SCAN_SATELLITE: {
        dvb = type;
        frontend_type = type;
        choose_satellite(satellite, this_channellist);

        /* find a dvb-s2 capable device using *some* channel */
        char p[] = {'H','V','L','R'};
        aChannel = new TChannel;
        size_t ch = 0;
        for(size_t i=0; i<sat_list[this_channellist].item_count; i++) {
           aChannel->Source = sat_list[this_channellist].source_id;
           aChannel->Frequency = sat_list[this_channellist].items[i].intermediate_frequency;
           aChannel->Polarization = p[sat_list[this_channellist].items[i].polarization];
           if (aChannel->ValidSatIf()) {
              ch = i;
              break;
              }
           }

        aChannel->Name         = "???";
        aChannel->Source       = sat_list[this_channellist].source_id;
        aChannel->West         = sat_list[this_channellist].west_east_flag == WEST_FLAG;
        aChannel->OrbitalPos   = aChannel->West ?
                                  BCDtoDecimal(0x3600) - BCDtoDecimal(sat_list[this_channellist].orbital_position) :
                                                         BCDtoDecimal(sat_list[this_channellist].orbital_position);
        aChannel->Frequency    = sat_list[this_channellist].items[ch].intermediate_frequency;
        aChannel->Polarization = p[sat_list[this_channellist].items[ch].polarization];
        aChannel->Symbolrate   = 27500;
        aChannel->FEC          = 23;
        aChannel->Modulation   = 5;
        aChannel->DelSys       = 1;
        aChannel->Rolloff      = 35;
        if ((dev = GetPreferredDevice(aChannel)) == nullptr) {
           dlog(0, "No DVB-S2 device available - trying fallback to DVB-S");
           if (MenuScanning) MenuScanning->SetStatus((status = 3));
           aChannel->Modulation = 2;
           aChannel->DelSys     = 0;
           caps_s2 = 0;
           if ((dev = GetPreferredDevice(aChannel)) == nullptr) {
              dlog(0, "No device available - exiting!");
              if (MenuScanning)
                 MenuScanning->SetStatus((status = 2));
              DeleteNullptr(aChannel);
              return;
              }
           }

        if (std::string(dev->DeviceName()).find("SAT>IP") == std::string::npos) {
           PrintDvbApi(s);
           dlog(5, s);
           if (! GetSatCapabilities(dev, &crAuto, &modAuto, &roAuto, &s2Support))
              dlog(0, "ERROR: Could not query capabilites.");
           }
        else {
           crAuto    = 0;
           modAuto   = 0;
           roAuto    = 0;
           s2Support = 1;
           }
        if (caps_s2) s2Support = 1;
        lDeviceName = dev->DeviceName();
        dlog(3, "frontend " + lDeviceName);
        if (MenuScanning)
           MenuScanning->SetDeviceName(lDeviceName);

        caps_inversion = 999;
        if (crAuto)
           caps_fec = 999;
        dlog(5, "DVB-S");
        if (s2Support) {
           dlog(5, "DVB-S2");
           caps_s2 = 1;
           }
        else
           wSetup.enable_s2 = false;

        // channel means here: transponder,
        // last channel == (item_count - 1) since we're counting from 0
        channel_max = sat_list[this_channellist].item_count - 1;
        // disable qam loop
        modulation_min = modulation_max = 0;
        // disable symbolrate loop
        dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
        // disable freq offset loop
        freq_offset_min = freq_offset_max = 0;
        break;
        }
     case SCAN_TERRCABLE_ATSC: {
        int atsc = 1 + wSetup.ATSC_type;
        frontend_type = type;
        choose_country(country, atsc, dvb, frontend_type, this_channellist);

        /* TODO: distinguish between atsc vsb && atsc qam */
        aChannel = new TChannel;
        aChannel->Source = "A";
        aChannel->Frequency = 474;
        aChannel->Modulation = 256;
        aChannel->Symbolrate = 6900;
        aChannel->Inversion = 0;
        aChannel->FEC = 0;
        aChannel->DelSys = 0;
        if ((dev = GetPreferredDevice(aChannel)) == nullptr) {
           dlog(0, "No device available - exiting!");
           if (MenuScanning) MenuScanning->SetStatus((status = 2));
           DeleteNullptr(aChannel);
           return;
           }

        if (std::string(dev->DeviceName()).find("SAT>IP") == std::string::npos) {
           PrintDvbApi(s);
           dlog(5, s);
           if (! GetAtscCapabilities(dev, &modAuto, &invAuto, &vsbSupport, &qamSupport))
              dlog(0, "ERROR: Could not query capabilites.");
           }
        else {
           modAuto    = 0;
           invAuto    = 0;
           vsbSupport = 1;
           qamSupport = 1;
           }
        lDeviceName = dev->DeviceName();
        dlog(3, "frontend " + lDeviceName);
        if (MenuScanning)
           MenuScanning->SetDeviceName(lDeviceName);

        if (invAuto)
           caps_inversion = 999;
        else {
           caps_inversion = 0;
           dlog(5, "I999 not supported, trying I" + IntToStr(caps_inversion) + ".");
           }
        if (vsbSupport)
           dlog(5, "VSB");
        if (qamSupport) {
           dlog(5, "QAM");
           caps_qam = 1;
           }
        switch (1 + wSetup.ATSC_type) {
           case ATSC_VSB:
              modulation_min = modulation_max = ATSC_VSB;
              break;
           case ATSC_QAM:
              modulation_min = modulation_max = ATSC_QAM;
              break;
           default:
              modulation_min = ATSC_VSB;
              modulation_max = ATSC_QAM;
              break;
           }
        // disable symbolrate loop
        dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
        break;
        }
     default:
        dlog(0, "ERROR: Unknown scan type " + IntToStr(type));
        return;
     } // end switch type

  if (lDeviceName.compare(0, 6, "SAT>IP") == 0)
     isSatip = true;
  if (MenuScanning)
     MenuScanning->SetStatus((status = 1));

  //count channels.
  switch(type) {
     case SCAN_SATELLITE:
     case SCAN_TRANSPONDER:
        initialTransponders = channel_max;
        break;
     default:
        // number depends on offset and symbolrates; counting in nested loops is easiest way.
        for(mod_parm = modulation_min; mod_parm <= modulation_max; mod_parm++)
           for(channel = channel_min; channel <= channel_max; channel++)
              for(offs = freq_offset_min; offs <= freq_offset_max; offs++)
                 for(sr_parm = dvbc_symbolrate_min; sr_parm <= dvbc_symbolrate_max; sr_parm++) {
                    if ((! chan_to_freq(channel, this_channellist)) ||
                        (freq_offset(channel, this_channellist, offs) == -1))
                       continue;
                    ++initialTransponders;
                    }
     } // switch(type)


  cChannel c;

  for(mod_parm = modulation_min; mod_parm <= modulation_max; mod_parm++) {
    for(channel = channel_min; channel <= channel_max; channel++) {
      for(offs = freq_offset_min; offs <= freq_offset_max; offs++)
        for(sr_parm = dvbc_symbolrate_min; sr_parm <= dvbc_symbolrate_max; sr_parm++) {
          if (!ActionAllowed())
             goto stop;

          switch (type) {
             case SCAN_TERRESTRIAL: {
                std::array<int,2> DelSys = {1,0}; // {T2,T}
                sys_parm = DelSys[mod_parm];      // NOTE: mod_parm is abused as 'system'
                if (thisSystem != sys_parm) {
                   thisSystem = sys_parm;
                   std::string Gen2(sys_parm, '2');
                   dlog(4, "Scanning DVB-T" + Gen2 + "...");
                   }

                if (sys_parm == 0) {
                   // DVB-T: no plp, skip plp loop.
                   streamid_parm = 0;
                   sr_parm = dvbc_symbolrate_max;
                   }
                else {
                   streamid_parm = sr_parm;  // NOTE: sr_parm is abused as 'plp'
                   }
                f = chan_to_freq(channel, this_channellist);
                if (!f)
                   continue; //skip unused channels

                if (freq_offset(channel, this_channellist, offs) == -1)
                   continue; //skip this one

                f += freq_offset(channel, this_channellist, offs);
                {
                int bHz = bandwidth(channel, this_channellist), bvdr = bHz / 1000000;
                if (bHz == 1712000) bvdr = 1712;

                if (this_bandwidth != bvdr) {
                   if (bvdr < 11)
                      dlog(4, "Scanning " + IntToStr(bvdr) + "MHz frequencies...");
                   else
                      dlog(4, "Scanning " + FloatToStr(bvdr/1000.0,1,2,false) + "MHz frequencies...");
                   }
                this_bandwidth = bvdr;
                }

                aChannel->Source = "T";
                aChannel->Frequency = f;
                aChannel->Symbolrate = 0;
                aChannel->Inversion = caps_inversion;
                aChannel->Bandwidth = this_bandwidth;
                aChannel->FEC = caps_fec;
                aChannel->FEC_low =  caps_fec;
                aChannel->Modulation = caps_qam;
                aChannel->DelSys = sys_parm;
                aChannel->Transmission = caps_transmission_mode;
                aChannel->Guard = caps_guard_interval;
                aChannel->Hierarchy = caps_hierarchy;
                aChannel->NID = 0;
                aChannel->TID = 0;
                aChannel->SID = 0;
                aChannel->RID = 0;
                aChannel->StreamId = streamid_parm;
                aChannel->SystemId = 0;

                aChannel->PrintTransponder(s);
                dlog(4, s);

                if (known_transponder(aChannel, false)) {
                   dlog(4, FloatToStr(aChannel->Frequency/1e6, 1, 3, false) +
                        "MHz: skipped (already known transponder)");
                   thisChannel++;
                   Progress();
                   continue;
                   }
                }
                break;
             case SCAN_CABLE:
                f = chan_to_freq(channel, this_channellist);
                if (!f)
                   continue; //skip unused channels

                if (freq_offset(channel, this_channellist, offs) == -1)
                   continue; //skip this one

                f += freq_offset(channel, this_channellist, offs);
                this_qam = caps_qam;
                if (qam_no_auto > 0) {
                   this_qam = dvbc_modulation(mod_parm);
                   if ((int) aChannel->Modulation != this_qam)
                      dlog(4, "searching M" + IntToStr(this_qam) + "...");
                   }
                else if (mod_parm > 0) {
                   continue; // demod supports qam_auto, and we had one loop with QAM_AUTO.
                   }

                aChannel->Source = "C";
                aChannel->Frequency = f / 1000;
                aChannel->Symbolrate = dvbc_symbolrate(sr_parm) / 1000;
                aChannel->Inversion = caps_inversion;
                aChannel->Bandwidth = 999;
                aChannel->FEC = caps_fec;
                aChannel->Modulation = this_qam;
                aChannel->DelSys = 0;
                aChannel->NID = 0;
                aChannel->TID = 0;
                aChannel->SID = 0;
                aChannel->RID = 0;

                aChannel->PrintTransponder(s);
                dlog(4, s);

                if (known_transponder(aChannel, false)) {
                   dlog(4, FloatToStr(aChannel->Frequency/1e3, 1, 3, false) +
                        "MHz: skipped (already known transponder)");
                   thisChannel++;
                   Progress();
                   continue;
                   }
                break;
             case SCAN_SATELLITE:
                {
                auto& sat = sat_list[this_channellist];
                auto& tp = sat.items[channel];
                aChannel->Source = sat.source_id;
                aChannel->Frequency  = tp.intermediate_frequency;
                aChannel->Symbolrate = tp.symbol_rate;
                aChannel->DelSys     = tp.modulation_system == 6 ? 1:0;
                aChannel->StreamId   = tp.stream_id;

                char p[] = {'H','V','L','R'};
                aChannel->Polarization = p[tp.polarization];

                int f[] = {0,12,23,34,45,56,67,78,89,999,35,910};
                aChannel->FEC = f[tp.fec_inner];

                int m[] = {2,16,32,64,128,256,999,10,11,5,6,7,12,0};
                aChannel->Modulation = m[tp.modulation_type];

                int r[] = {35,20,25,999};
                aChannel->Rolloff = r[tp.rolloff];

                aChannel->Pilot = 999;
                aChannel->NID = 0;
                aChannel->TID = 0;
                aChannel->SID = 0;
                aChannel->RID = 0;
                }

                if (! aChannel->ValidSatIf())
                   continue;
 
                aChannel->Print(s);
                dlog(4, s);

                ///orbital_position = sat_list[this_channellist].orbital_position;
                ///west_east_flag   = sat_list[this_channellist].west_east_flag;
                if (sat_list[this_channellist].items[channel].modulation_system == 6) {
                   if (!(caps_s2) || (wSetup.enable_s2 == 0)) {
                      dlog(4, IntToStr(sat_list[this_channellist].items[channel].intermediate_frequency) +
                              ": skipped (no S2 support)");
                      thisChannel++;
                      Progress();
                      continue;
                      }
                   }
                
                if (known_transponder(aChannel, false)) {
                   dlog(4, FloatToStr(aChannel->Frequency/1e0, 1, 3, false) +
                        ": skipped (already known transponder)");
                   thisChannel++;
                   continue;
                   }
                break;
             case SCAN_TERRCABLE_ATSC:
                switch(mod_parm) {
                   case ATSC_VSB:
                      this_atsc = 10;
                      f = chan_to_freq(channel, ATSC_VSB);
                      if (!f)
                         continue; //skip unused channels

                      if (freq_offset(channel, ATSC_VSB, offs) == -1)
                         continue; //skip this one

                      f += freq_offset(channel, ATSC_VSB, offs);
                      break;
                   case ATSC_QAM:
                      this_atsc = 256;
                      f = chan_to_freq(channel, ATSC_QAM);
                      if (!f)
                         continue; //skip unused channels

                      if (freq_offset(channel, ATSC_QAM, offs) == -1)
                         continue; //skip this one

                      f += freq_offset(channel, ATSC_QAM, offs);
                      break;
                   default:
                      dlog(0, "unknown atsc modulation id " + IntToStr(mod_parm));
                      return;
                   } // end switch mod_parm
                //fixme: vsb vs qam here
                aChannel->Source = "A";
                aChannel->Frequency = f / 1000;
                aChannel->Symbolrate = dvbc_symbolrate(sr_parm) / 1000;
                aChannel->Modulation = this_atsc;
                aChannel->Inversion = caps_inversion;
                aChannel->FEC = caps_fec;
                aChannel->DelSys = 0;
                aChannel->NID = 0;
                aChannel->TID = 0;
                aChannel->SID = 0;
                aChannel->RID = 0;

                aChannel->PrintTransponder(s);
                dlog(4, s);

                if (known_transponder(aChannel, false)) {
                   dlog(4, FloatToStr(aChannel->Frequency/1e6, 1, 3, false) +
                        "MHz M" + IntToStr(this_atsc) + ": skipped (already known transponder)");
                   thisChannel++;
                   Progress();
                   continue;
                   }
                break;

             case SCAN_TRANSPONDER:
                aChannel->PrintTransponder(s);
                dlog(4, s);
                break;
             default:;
             } // end switch type
          ++thisChannel;
          lStrength = 0;
          Progress();
          lTransponder = s.c_str();
          if (MenuScanning) {
             MenuScanning->SetTransponder(aChannel);
             }
          aChannel->Tested = false;
          int nid = aChannel->NID;
          int sid = aChannel->SID;
          aChannel->NID = 0x2000;
          aChannel->SID = 0x2000;
          aChannel->VdrChannel(c);
          aChannel->NID = nid;
          aChannel->SID = sid;          
          dev->SwitchChannel(&c, false);

          {
          bool lock;

          if (MenuScanning)
             MenuScanning->SetStr(0, false);

          mSleep(wSetup.SignalWaitTime * 1000);
          if (isSatip or GetFrontendStatus(dev) & FE_HAS_SIGNAL) 
             lock = dev->HasLock(wSetup.LockTimeout * 1000);
          else
             lock = false;

          if (lock) {
             lStrength = std::min((size_t)dev->SignalStrength(), (size_t)100);
             if (MenuScanning)
                MenuScanning->SetStr(lStrength, lock);
             StateMachine = new cStateMachine(dev, aChannel, useNit, this);
             while(StateMachine && StateMachine->Active())
                mSleep(100);
             DeleteNullptr(StateMachine);
             }
           }


          if (dev)
             dev->DetachAllReceivers();
          } // end loop sr_parm
       } // end loop channel
    } // end loop mod_parm


stop:
  AddChannels();
  if (MenuScanning)
     MenuScanning->SetStatus((status = 0));

  if (dev)
     dev->DetachAllReceivers();

  //Channels.ReNumber();
  SetShouldstop(true);
  dlog(3, "leaving scanner");
  Cancel();
  Scanner = nullptr;
}



/* Here i cannot avoid anymore dealing with vdrs lists and
 * channel classes. So the real complicated stuff is here..
 */
#include <vdr/channels.h>

void cScanner::AddChannels(void) {
  cStateKey WriteState;
  cChannels* WChannels = (cChannels*) cChannels::GetChannelsWrite(WriteState, 30000);

  extern TChannels NewChannels;

  if (!WChannels)
     return;

  for(int i = 0; i < WChannels->Count(); i++) {
     const cChannel* ch = WChannels->Get(i);
     TChannel* newCh = nullptr;
     int source = 0;

     // is 'ch' known in NewChannels?
     for(int idx = 0; idx < NewChannels.Count(); idx++) {
        if (!source)
            source = cSource::FromString(NewChannels[idx]->Source.c_str());

        if (ch->Nid() != NewChannels[idx]->ONID or
            ch->Tid() != NewChannels[idx]->TID or
            ch->Sid() != NewChannels[idx]->SID or
            ch->Source() != cSource::FromString(NewChannels[idx]->Source.c_str()))
           continue;

        // this channel is already known.
        newCh = NewChannels[idx];
        break;
        }


     // existing channel not found by IDs
     if (wSetup.scan_remove_invalid and !newCh and ch->Source() == source) {
        dlog(4, "remove invalid channel '" + std::string(*ch->ToText()) + "'");
        WChannels->Del((cChannel*) ch);
        i--;
        continue;
        }

     // update existing
     if (wSetup.scan_update_existing and newCh) {
        std::string s;
        newCh->Print(s);
        if (s != *ch->ToText()) {
           ((cChannel*) ch)->Parse(s.c_str());
           dlog(4, "updated channel '" + std::string(*ch->ToText()) + "'");
           }
        }
     }

  if (wSetup.scan_append_new)
  for(int i=0; i<NewChannels.Count(); i++) {
     TChannel* n = NewChannels[i];
     const cChannel* old = nullptr;

     for(old = WChannels->First(); old; old = WChannels->Next(old)) {
        if (old->Nid() == n->ONID and
            old->Tid() == n->TID and
            old->Sid() == n->SID and
            *cSource::ToString(old->Source()) == n->Source) {
           break;
           }
        }

     if (!old) {
        std::string s;
        cChannel* c = new cChannel;
        n->Print(s);
        c->Parse(s.c_str());
        dlog(4, "Add channel '" + s + "'");
        WChannels->Add(c);
        }           
     }
  WChannels->ReNumber();
  WriteState.Remove();
}
