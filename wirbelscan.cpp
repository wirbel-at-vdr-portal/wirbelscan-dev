/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#include <string>
#include <vector>
#include <sstream>
#include <cctype>        // std::toupper()
#include <vdr/plugin.h>
#include <vdr/i18n.h>
#include "common.h"      // wSetup
#include "wirbelscan.h"
#include "wirbelscan_services.h"
#include "menusetup.h"
#include "countries.h"
#include "satellites.h"

class cScanner;

extern int channelcount;     // menusetup.c
extern int nextTransponders; // scanfilter.c
extern cScanner* Scanner;

const char* WIRBELSCAN_VERSION        = "2021.10.xx";
const char* WIRBELSCAN_DESCRIPTION    = "DVB channel scan for VDR";
const char* WIRBELSCAN_MAINMENUENTRY  = nullptr; /* main menu -> use wirbelscancontrol plugin */
cPluginWirbelscan* thisPlugin;

const char* cPluginWirbelscan::Version(void) {
  return WIRBELSCAN_VERSION;
}

const char* cPluginWirbelscan::Description(void) {
  return WIRBELSCAN_DESCRIPTION; //return I18nTranslate(WIRBELSCAN_DESCRIPTION, "vdr-wirbelscan");
}

const char* cPluginWirbelscan::MainMenuEntry(void) {
  return WIRBELSCAN_MAINMENUENTRY;
}

// constructor
cPluginWirbelscan::cPluginWirbelscan(void) {
  thisPlugin = this;
  servicetype("", true);
}

// destructor
cPluginWirbelscan::~cPluginWirbelscan() {
}

// Return a string that describes all known command line options.
const char* cPluginWirbelscan::CommandLineHelp(void) {
  return nullptr;
}

// Implement command line argument processing here if applicable.
bool cPluginWirbelscan::ProcessArgs(int argc, char* argv[]) {
  return true;
}

// Initialize any background activities the plugin shall perform.
bool cPluginWirbelscan::Initialize(void) {
  return true;
}

// Start any background activities the plugin shall perform.
bool cPluginWirbelscan::Start(void) {
  return true;
}

// Stop any background activities the plugin shall perform.
void cPluginWirbelscan::Stop(void) {
  stopScanners();
}

// Perform any cleanup or other regular tasks.
void cPluginWirbelscan::Housekeeping(void) {
}

// Perform actions in the context of the main program thread.
void cPluginWirbelscan::MainThreadHook(void) {
}

// Return a message string if shutdown should be postponed
cString cPluginWirbelscan::Active(void) {
  return nullptr;
}

// Perform the action when selected from the main VDR menu.
cOsdObject* cPluginWirbelscan::MainMenuAction(void) { 
  return nullptr;
}

// Return a setup menu in case the plugin supports one.
cMenuSetupPage* cPluginWirbelscan::SetupMenu(void) {
  return new cMenuScanning();
}

// read back plugins settings.
bool cPluginWirbelscan::SetupParse(const char* Name, const char* Value) {
  std::string name(Name);
  if      (name == "verbosity")        wSetup.verbosity            = constrain(std::stoi(Value), 0, 6);
  else if (name == "logFile")          wSetup.logFile              = constrain(std::stoi(Value), 0, STDERR);
  else if (name == "DVB_Type")         wSetup.DVB_Type             = constrain(std::stoi(Value), SCAN_TERRESTRIAL, SCAN_TRANSPONDER);
  else if (name == "DVBT_Inversion")   wSetup.DVBT_Inversion       = constrain(std::stoi(Value), 0, 1);
  else if (name == "DVBC_Inversion")   wSetup.DVBC_Inversion       = constrain(std::stoi(Value), 0, 1);
  else if (name == "DVBC_Symbolrate")  wSetup.DVBC_Symbolrate      = constrain(std::stoi(Value), 0, 16);
  else if (name == "DVBC_Network_PID") wSetup.DVBC_Network_PID     = constrain(std::stoi(Value), 0x10, 0xFFFF);
  else if (name == "DVBC_QAM")         wSetup.DVBC_QAM             = constrain(std::stoi(Value), 0, 4);
  else if (name == "CountryIndex")     wSetup.CountryIndex         = constrain(std::stoi(Value), 0, (int)COUNTRY::country_count()-1);
  else if (name == "SatIndex")         wSetup.SatIndex             = constrain(std::stoi(Value), 0, (int)sat_count()-1);
  else if (name == "enable_s2")        wSetup.enable_s2            = constrain(std::stoi(Value), 0, 1);
  else if (name == "ATSC_type")        wSetup.ATSC_type            = constrain(std::stoi(Value), 0, 2);
  else if (name == "scanflags")        wSetup.scanflags            = constrain(std::stoi(Value), 0, 255);
  else if (name == "user0")            wSetup.user[0]              = std::stol(Value);
  else if (name == "user1")            wSetup.user[1]              = std::stol(Value);
  else if (name == "user2")            wSetup.user[2]              = std::stol(Value);
  else if (name == "ri")               wSetup.scan_remove_invalid  = constrain(std::stoi(Value), 0, 1);
  else if (name == "ue")               wSetup.scan_update_existing = constrain(std::stoi(Value), 0, 1);
  else if (name == "an")               wSetup.scan_append_new      = constrain(std::stoi(Value), 0, 1);
  else return false;
  return true;
}

using namespace WIRBELSCAN_SERVICE;

void cPluginWirbelscan::StoreSetup(void) {
  if (wSetup.DVB_Type > 5)
     wSetup.DVB_Type = (int) G(wSetup.user[1],3,29);

  SetupStore("verbosity",       wSetup.verbosity);
  SetupStore("logFile",         wSetup.logFile);
  SetupStore("DVB_Type",        wSetup.DVB_Type);
  SetupStore("DVBT_Inversion",  wSetup.DVBT_Inversion);
  SetupStore("DVBC_Inversion",  wSetup.DVBC_Inversion);
  SetupStore("DVBC_Symbolrate", wSetup.DVBC_Symbolrate);
  SetupStore("DVBC_QAM",        wSetup.DVBC_QAM);
  SetupStore("DVBC_Network_PID",wSetup.DVBC_Network_PID);
  SetupStore("CountryIndex",    wSetup.CountryIndex);
  SetupStore("SatIndex",        wSetup.SatIndex);
  SetupStore("enable_s2",       wSetup.enable_s2);
  SetupStore("ATSC_type",       wSetup.ATSC_type);
  SetupStore("scanflags",       wSetup.scanflags);
  SetupStore("user0",           wSetup.user[0]);
  SetupStore("user1",           wSetup.user[1]);
  SetupStore("user2",           wSetup.user[2]);
  SetupStore("ri",              wSetup.scan_remove_invalid);
  SetupStore("ue",              wSetup.scan_update_existing);
  SetupStore("an",              wSetup.scan_append_new);
  Setup.Save();
}

/* convert service strings to zero based int, -1 on error
 */
int cPluginWirbelscan::servicetype(const char* id, bool init) {
  static std::vector<std::string> services;

  if (init) {
     std::string s(SPlugin);
     services.clear();
     services.push_back(s +         SInfo);
     services.push_back(s + "Get" + SStatus);
     services.push_back(s +         SCommand);
     services.push_back(s + "Get" + SSetup);
     services.push_back(s + "Set" + SSetup);
     services.push_back(s + "Get" + SSat);
     services.push_back(s + "Get" + SCountry);
     services.push_back(s + "Get" + SUser);
     services.push_back(s + "Set" + SUser);
     services.push_back(s +       + SExport);
     }

  for(size_t i=0; i<services.size(); i++) {
     if (services[i] == id)
        return i;
     }
  return -1;
}

// Handle custom service requests from other plugins
bool cPluginWirbelscan::Service(const char* id, void* Data) {
  switch(servicetype(id)) {
     case 0: { // info
        if (! Data) return true; // check for support.
        cWirbelscanInfo* info = (cWirbelscanInfo*) Data;
        info->PluginVersion  = WIRBELSCAN_VERSION;
        info->CommandVersion = SCommand;
        info->StatusVersion  = SStatus;
        info->SetupVersion   = SSetup;
        info->CountryVersion = SCountry;
        info->SatVersion     = SSat;
        info->UserVersion    = SUser;   // 0.0.5-pre12b
        info->reserved2      = WIRBELSCAN_VERSION; // may change later.
        info->reserved3      = WIRBELSCAN_VERSION; // may change later.
        return true;
        }
     case 1: { // status
        if (! Data) return true; // check for support.
        cWirbelscanStatus* s = (cWirbelscanStatus*) Data;
        if (Scanner)
           s->status = StatusScanning;
        else
           s->status = StatusStopped;
        memset(s->curr_device, 0, 256);
        strcpy(s->curr_device, lDeviceName.size()? lDeviceName.c_str():"none");
        memset(s->transponder, 0, 256);
        strcpy(s->transponder, lTransponder.length()? lTransponder.c_str():"none");
        s->progress = s->status == StatusScanning?lProgress:0;
        s->strength = s->status == StatusScanning?lStrength:0;
        s->numChannels = 0;              // Channels.Count(); // not possible any longer.
        s->newChannels = channelcount;   // ((Channels.Count() - channelcount) > 0) && channelcount?Channels.Count() - channelcount:0;
        s->nextTransponders = nextTransponders;
        return true;
        }
     case 2: { // command
        if (! Data) return true; // check for support.
        cWirbelscanCmd* request = (cWirbelscanCmd*) Data;
        switch (request->cmd) {
           case CmdStartScan:
              request->replycode = DoScan(wSetup.DVB_Type);
              break;
           case CmdStopScan:
              DoStop();
              request->replycode = true;
              break;
           case CmdStore:
              StoreSetup();
              request->replycode = true;
              break;
           default:
              request->replycode = false;
              return false;
           }
        return true;
        }
     case 3: { // get setup
        if (! Data) return true; // check for support.
        cWirbelscanScanSetup* d = (cWirbelscanScanSetup*) Data;
        d->verbosity       = wSetup.verbosity;
        d->logFile         = wSetup.logFile;
        d->DVB_Type        = wSetup.DVB_Type;
        d->DVBT_Inversion  = wSetup.DVBT_Inversion;
        d->DVBC_Inversion  = wSetup.DVBC_Inversion;
        d->DVBC_Symbolrate = wSetup.DVBC_Symbolrate;
        d->DVBC_QAM        = wSetup.DVBC_QAM;
        d->CountryId       = wSetup.CountryIndex;
        d->SatId           = wSetup.SatIndex;
        d->scanflags       = wSetup.scanflags;
        d->ATSC_type       = wSetup.ATSC_type;
        return true;
        }
     case 4: { // set setup
        if (! Data) return true; // check for support.
        cWirbelscanScanSetup* d = (cWirbelscanScanSetup*) Data;
        wSetup.verbosity       = d->verbosity;
        wSetup.logFile         = d->logFile;
        wSetup.DVB_Type        = (int) d->DVB_Type;
        wSetup.DVBT_Inversion  = d->DVBT_Inversion;
        wSetup.DVBC_Inversion  = d->DVBC_Inversion;
        wSetup.DVBC_Symbolrate = d->DVBC_Symbolrate;
        wSetup.DVBC_QAM        = d->DVBC_QAM;
        wSetup.CountryIndex    = d->CountryId;
        wSetup.SatIndex        = d->SatId;
        wSetup.scanflags       = d->scanflags;
        wSetup.ATSC_type       = d->ATSC_type;
        return true;
        }
     case 5: { // get sat
        if (! Data) return true; // check for support.
        cPreAllocBuffer* b = (cPreAllocBuffer*) Data;
        SListItem* l = b->buffer;
        b->count = 0;
        if (b->size < sat_count()) {
           b->size = sat_count();
           return true;
           }
        for(size_t i=0; i<sat_count(); i++) {
           memset(&l[i], 0, sizeof(SListItem));
           l[i].id = sat_list[i].id;
           strcpy(l[i].short_name, sat_list[i].short_name);
           strcpy(l[i].full_name, sat_list[i].full_name);
           b->count++;
           }
        return true;
        }
     case 6: { // get country
        if (! Data) return true; // check for support.
        cPreAllocBuffer* b = (cPreAllocBuffer*) Data;
        SListItem* l = b->buffer;
        b->count = 0;
        if (b->size < COUNTRY::country_count()) {
           b->size = COUNTRY::country_count();
           return true;
           }
        for(size_t i=0; i<COUNTRY::country_count(); i++) {
           memset(&l[i], 0, sizeof(SListItem));
           l[i].id = COUNTRY::country_list[i].id;
           strcpy(l[i].short_name, COUNTRY::country_list[i].short_name);
           strcpy(l[i].full_name, COUNTRY::country_list[i].full_name);
           b->count++;
           }
        return true;
        }
     case 7: { // get user
        if (! Data) return true; // check for support
        *((uint32_t*) Data + 0) = wSetup.user[0];
        *((uint32_t*) Data + 1) = wSetup.user[1];
        *((uint32_t*) Data + 2) = wSetup.user[2];
        return true;
        }
     case 8: { // set user
        if (! Data) return true; // check for support
        wSetup.user[0] = *((uint32_t*) Data + 0);
        wSetup.user[1] = *((uint32_t*) Data + 1);
        wSetup.user[2] = *((uint32_t*) Data + 2);
        return true;
        }
     case 9: { // Export
        if (! Data) return true; // check for support
        extern TChannels NewChannels;
        std::vector<TChannel>* list = (std::vector<TChannel>*) Data;
        for(int idx = 0; idx < NewChannels.Count(); ++idx) {
           TChannel t = *NewChannels[idx];
           list->push_back(t);
           }
        return true;
        }
     default:
        return false;
     }
}

const char** cPluginWirbelscan::SVDRPHelpPages(void) {
  static const char* SVDRHelp[] = {
    "S_START\n"
    "    Start scan",
    "S_STOP\n"
    "    Stop scan(s) (if any)",
    "S_TERR\n"
    "    Start DVB-T scan",
    "S_CABL\n"
    "    Start DVB-C scan",
    "S_SAT\n"
    "    Start DVB-S/S2 scan",
    "SETUP <verb:log:type:inv_t:inv_c:srate:qam:cidx:sidx:s2:atsc:flags>\n"
    "    verb   verbostity (0..5)\n"
    "    log    logfile (0=OFF, 1=stdout, 2=syslog)\n"
    "    type   scan type\n"
    "           (0=DVB-T, 1=DVB-C, 2=DVB-S/S2, 5=ATSC)\n"
    "    inv_t  DVB-T inversion\n"
    "           (0=AUTO/OFF, 1=AUTO/ON)\n"
    "    inv_c  DVB-C inversion\n"
    "           (0=AUTO/OFF, 1=AUTO/ON)\n"
    "    srate  DVB-C Symbolrate (0..15)\n"
    "    qam    DVB-C modulation\n"
    "           (0=AUTO, 1=QAM64, 2=QAM128, 3=QAM256, 4=ALL)\n"
    "    cidx   country list index\n"
    "    sidx   satellite list index\n"
    "    s2     enable DVB-S2 (0=OFF, 1=ON)\n"
    "    atsc   ATSC scan type\n"
    "           (0=VSB, 1=QAM, 2=VSB+QAM)\n"
    "    flags  bitwise flag of\n"
    "           TV=1, RADIO=2, FTA=4, SCRAMBLED=8, HDTV=16",
    "STORE\n"
    "    Store current setup",
    "LSTC\n"
    "    list countries",
    "LSTS\n"
    "    list satellites",
    "QUERY\n"
    "    return plugin version, current setup and service versions",
    nullptr
    };
  return SVDRHelp;
}

// process svdrp commands.
cString cPluginWirbelscan::SVDRPCommand(const char* Command, const char* Option, int& ReplyCode) {
  auto uppercase = [](std::string& s) { for(auto& c:s) c = std::toupper(c); };
  auto split = [](const std::string s, const char delim) {
     std::stringstream ss(s);
     std::vector<std::string> result;
     std::string i;
     while(std::getline(ss, i, delim))
        result.push_back(i);
     return result;
     };
  std::string cmd(Command);
  uppercase(cmd);

  if      (cmd == "S_TERR" ) { return DoScan(wSetup.DVB_Type = SCAN_TERRESTRIAL)   ? "DVB-T scan started"     : "Could not start DVB-T scan.";    }
  else if (cmd == "S_CABL" ) { return DoScan(wSetup.DVB_Type = SCAN_CABLE)         ? "DVB-C scan started"     : "Could not start DVB-C scan.";    }
  else if (cmd == "S_SAT"  ) { return DoScan(wSetup.DVB_Type = SCAN_SATELLITE)     ? "DVB-S scan started"     : "Could not start DVB-S scan.";    }
  else if (cmd == "S_START") { return DoScan(wSetup.DVB_Type)              ? "starting scan"          : "Could not start scan.";          }
  else if (cmd == "S_STOP" ) { DoStop();       return "stopping scan(s)";  }
  else if (cmd == "STORE"  ) { StoreSetup();   return "setup stored.";     }

  else if (cmd == "SETUP") {
     std::vector<std::string> items;
     if (Option and *Option) items = split(Option, ':');

     bool valid = items.size() == 12;
     for(auto i:items)
        if (i.empty() or i.find_first_not_of("0123456789") != std::string::npos)
           valid = false;

     if (not valid) {
        ReplyCode = 501;
        return "couldnt parse setup string.";
        }
     wSetup.verbosity        = std::stol(items[0]);
     wSetup.logFile          = std::stol(items[1]);
     wSetup.DVB_Type         = std::stol(items[2]);
     wSetup.DVBT_Inversion   = std::stol(items[3]);
     wSetup.DVBC_Inversion   = std::stol(items[4]);
     wSetup.DVBC_Symbolrate  = std::stol(items[5]);
     wSetup.DVBC_QAM         = std::stol(items[6]);
     wSetup.CountryIndex     = std::stol(items[7]);
     wSetup.SatIndex         = std::stol(items[8]);
     wSetup.enable_s2        = std::stol(items[9]);
     wSetup.ATSC_type        = std::stol(items[10]);
     wSetup.scanflags        = std::stol(items[11]);

     std::string s = "changed setup to " + 
         IntToStr(wSetup.verbosity)       + ':' +
         IntToStr(wSetup.logFile)         + ':' +
         IntToStr(wSetup.DVB_Type)        + ':' +
         IntToStr(wSetup.DVBT_Inversion)  + ':' +
         IntToStr(wSetup.DVBC_Inversion)  + ':' +
         IntToStr(wSetup.DVBC_Symbolrate) + ':' +
         IntToStr(wSetup.DVBC_QAM)        + ':' +
         IntToStr(wSetup.CountryIndex)    + ':' +
         IntToStr(wSetup.SatIndex)        + ':' +
         IntToStr(wSetup.enable_s2)       + ':' +
         IntToStr(wSetup.ATSC_type)       + ':' +
         IntToStr(wSetup.scanflags);
     return s.c_str();
     }

  else if (cmd == "QUERY") {
     std::string s;
     s = "plugin version: " + std::string(WIRBELSCAN_VERSION) + '\n' +
         IntToStr(wSetup.verbosity)       + ':' +
         "current setup:  " +
         IntToStr(wSetup.verbosity)       + ':' +
         IntToStr(wSetup.logFile)         + ':' +
         IntToStr(wSetup.DVB_Type)        + ':' +
         IntToStr(wSetup.DVBT_Inversion)  + ':' +
         IntToStr(wSetup.DVBC_Inversion)  + ':' +
         IntToStr(wSetup.DVBC_Symbolrate) + ':' +
         IntToStr(wSetup.DVBC_QAM)        + ':' +
         IntToStr(wSetup.CountryIndex)    + ':' +
         IntToStr(wSetup.SatIndex)        + ':' +
         IntToStr(wSetup.enable_s2)       + ':' +
         IntToStr(wSetup.ATSC_type)       + ':' +
         IntToStr(wSetup.scanflags)       + '\n' +
         "commands api:   " + std::string(SCommand) + "\n"
         "status api:     " + std::string(SStatus)  + "\n"
         "setup api:      " + std::string(SSetup)   + "\n"
         "country api:    " + std::string(SCountry) + "\n"
         "sat api:        " + std::string(SSat)     + "\n"
         "user api:       " + std::string(SUser);
     return s.c_str();
     }

  else if (cmd == "LSTC") {
     std::stringstream ss;
     for(size_t i=0; i<COUNTRY::country_count(); i++)
        ss << IntToStr(COUNTRY::country_list[i].id) << ':'
           << COUNTRY::country_list[i].short_name   << ':'
           << COUNTRY::country_list[i].full_name    << '\n';
     return ss.str().c_str();
     }

  else if (cmd == "LSTS") {
     std::stringstream ss;
     for(size_t i=0; i<sat_count(); i++)
        ss << IntToStr(sat_list[i].id) << ':'
           << sat_list[i].short_name   << ':'
           << sat_list[i].full_name    << '\n';
     return ss.str().c_str();
     }

  return nullptr;
}

#ifndef STATIC_PLUGINS
VDRPLUGINCREATOR(cPluginWirbelscan);
#endif
