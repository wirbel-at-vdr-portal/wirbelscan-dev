/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#include <string>
#include <vector>
#include <array>
#include <algorithm>    // std::min()
#include <ctime>
#include <vdr/config.h>
#include "menusetup.h"
#include "common.h"
#include "satellites.h"
#include "countries.h"
#include "wirbelscan.h"
#include "scanner.h"
#include "common.h"

#define LOGLEN 8

using namespace COUNTRY;
extern cScanner* Scanner;

#undef tr
#define tr(str) (str)

std::array<const char*,7>  DVB_Types   = {"DVB-T/T2","DVB-C","DVB-S/S2","RESERVE1","RESERVE2","ATSC", "no device found"};
std::array<const char*,4>  logfiles    = {"Off","stdout","syslog","stderr"};
std::array<const char*,17> Symbolrates = {tr("AUTO"),"6900","6875","6111","6250",
                                          "6790","6811","5900","5000","3450","4000",
                                          "6950","7000","6952","5156","5483",tr("ALL (slow)")};
std::array<const char*,5>  Qams        = {tr("AUTO"),"64","128","256",tr("ALL (slow)")};
std::array<const char*,2>  inversions  = {tr("AUTO/OFF"),tr("AUTO/ON")};
std::array<const char*,3>  atsc_types  = {"VSB (aerial)","QAM (cable)","VSB + QAM (aerial + cable)"};
std::array<const char*,5>  st          = {"STOP","RUN","No device available - exiting!","No gen2 device available - trying gen1 device"," "};

std::vector<const char*> SatNames;
std::vector<const char*> CountryNames;

std::array<std::string, 4> flagslo = {"don\'t add channels", "TV", "Radio", "TV + Radio"};  
std::array<std::string, 4> flagshi = {"don\'t add channels", "Free to Air", "Scrambled", "Free to Air + Scrambled"};


cMenuScanning* MenuScanning    = nullptr;   // pointer to actual menu
cOsdItem*      DeviceUsed      = nullptr;
cOsdItem*      Progress        = nullptr;
cOsdItem*      CurrTransponder = nullptr;
cOsdItem*      Str             = nullptr;
cOsdItem*      ChanAdd         = nullptr;
cOsdItem*      ChanNew         = nullptr;
cOsdItem*      ScanType        = nullptr;
cOsdItem*      LogMsg[LOGLEN];


int channelcount = 0;
int lProgress = 0;
int lStrength = 0;
size_t lStatus = 0;
std::string lTransponder;
std::string deviceName;
time_t timestamp;


bool ScanAvailable(void) {
  return wSetup.systems[SCAN_TERRESTRIAL] ||
         wSetup.systems[SCAN_CABLE] ||
         wSetup.systems[SCAN_SATELLITE] ||
         wSetup.systems[SCAN_TERRCABLE_ATSC];
}

bool CableAvailable(void) {
  return wSetup.systems[SCAN_CABLE];
}

bool TerrAvailable(void) {
  return wSetup.systems[SCAN_TERRESTRIAL];
}

bool SatAvailable(void) {
  return wSetup.systems[SCAN_SATELLITE];
}

bool AtscAvailable(void) {
  return wSetup.systems[SCAN_TERRCABLE_ATSC];
}

bool TerrCableAvailable(void) {
  return wSetup.systems[SCAN_TERRESTRIAL] ||
         wSetup.systems[SCAN_CABLE] ||
         wSetup.systems[SCAN_TERRCABLE_ATSC];
}


/*******************************************************************************
 * class cMenuSettings
 ******************************************************************************/
class cMenuSettings : public cMenuSetupPage {
private:
  int scan_tv;
  int scan_radio;
  int scan_fta;
  int scan_scrambled;
protected:
  void AddCategory(std::string category);
  void Store(void);
public:
  cMenuSettings(void);
  ~cMenuSettings(void) {};
  virtual eOSState ProcessKey(eKeys Key);
};


cMenuSettings::cMenuSettings(void) {
  // devices may have changed meanwhile
  wSetup.InitSystems();

  if (not ScanAvailable()) {
     AddCategory("NO DEVICES FOUND.");
     return;
     }       

  scan_tv        = (wSetup.scanflags & SCAN_TV       ) > 0;
  scan_radio     = (wSetup.scanflags & SCAN_RADIO    ) > 0;
  scan_scrambled = (wSetup.scanflags & SCAN_SCRAMBLED) > 0;
  scan_fta       = (wSetup.scanflags & SCAN_FTA      ) > 0;

  if (SatNames.empty()) {
     SatNames.reserve(sat_count());
     for(size_t i=0; i<sat_count(); i++)
        SatNames.push_back(sat_list[i].full_name);
     }

  if (CountryNames.empty()) {
     CountryNames.reserve(country_count());
     for(size_t i=0; i<country_count(); i++)
        CountryNames.push_back(country_list[i].full_name);
     }

  SetSection(tr("Setup"));
  AddCategory(tr("General"));
  Add(new cMenuEditStraItem(tr("Source Type"),        &wSetup.DVB_Type,  DVB_Types.size()-1, DVB_Types.data()));
  Add(new cMenuEditIntItem (tr("verbosity"),          &wSetup.verbosity, 0, 6));
  Add(new cMenuEditStraItem(tr("logfile"),            &wSetup.logFile,   logfiles.size(), logfiles.data()));

  AddCategory(tr("Channels"));
  Add(new cMenuEditBoolItem(tr("TV channels"),        &scan_tv));
  Add(new cMenuEditBoolItem(tr("Radio channels"),     &scan_radio));
  Add(new cMenuEditBoolItem(tr("FTA channels"),       &scan_fta));
  Add(new cMenuEditBoolItem(tr("Scrambled channels"), &scan_scrambled));

  if (TerrCableAvailable()) {
     AddCategory(tr("Cable and Terrestrial"));
     Add(new cMenuEditStraItem(tr("Country"),             &wSetup.CountryIndex,     CountryNames.size(), CountryNames.data()));
     if (CableAvailable()) {
        Add(new cMenuEditStraItem(tr("Cable Inversion"),  &wSetup.DVBC_Inversion,   inversions.size(), inversions.data()));
        Add(new cMenuEditStraItem(tr("Cable Symbolrate"), &wSetup.DVBC_Symbolrate,  Symbolrates.size(), Symbolrates.data()));
        Add(new cMenuEditStraItem(tr("Cable modulation"), &wSetup.DVBC_QAM,         Qams.size(), Qams.data()));
        Add(new cMenuEditIntItem (tr("Cable Network PID"),&wSetup.DVBC_Network_PID, 16, 0xFFFE, "AUTO"));
        }
     if (TerrAvailable())
        Add(new cMenuEditStraItem(tr("Terr  Inversion"),  &wSetup.DVBT_Inversion,   inversions.size(), inversions.data()));

     if (AtscAvailable())
        Add(new cMenuEditStraItem(tr("ATSC  Type"),       &wSetup.ATSC_type,        atsc_types.size(), atsc_types.data()));

     }

  if (SatAvailable()) {
     AddCategory(tr("Satellite"));
     Add(new cMenuEditStraItem(tr("Satellite"),        &wSetup.SatIndex, SatNames.size(), SatNames.data()));
     Add(new cMenuEditBoolItem(tr("DVB-S2"),           &wSetup.enable_s2));
     }

  AddCategory(tr("Scan Mode"));
  Add(new cMenuEditBoolItem(tr("remove invalid channels"),   &wSetup.scan_remove_invalid));
  Add(new cMenuEditBoolItem(tr("update existing channels"),  &wSetup.scan_update_existing));
  Add(new cMenuEditBoolItem(tr("append new channels"),       &wSetup.scan_append_new));
}


void cMenuSettings::Store(void) {
  wSetup.scanflags  = scan_tv       ?SCAN_TV       :0;
  wSetup.scanflags |= scan_radio    ?SCAN_RADIO    :0;
  wSetup.scanflags |= scan_scrambled?SCAN_SCRAMBLED:0;
  wSetup.scanflags |= scan_fta      ?SCAN_FTA      :0;
  wSetup.update = true;
}


void cMenuSettings::AddCategory(std::string category) {
  Add(new cOsdItem(("---------------  " + category).c_str()));
}


eOSState cMenuSettings::ProcessKey(eKeys Key) {
  int direction = 0;
  eOSState state = cMenuSetupPage::ProcessKey(Key);
  switch(Key) {
     case kLeft:
        direction = -1;
        break; 
     case kRight:
        direction = 1;
        break; 
     case kOk:
     case kBack:
        thisPlugin->StoreSetup();
        wSetup.update = true;
        state=osBack;
        break;
     default:;
     }
  if (state == osUnknown) {
     switch(Key) {
        case kGreen:
        case kRed:
        case kBlue:
        case kYellow:
           state=osContinue;
           break;
        default:;
        }
     }

  if (not ScanAvailable()) {
     // no devices found; recursive call until we reach SCAN_NO_DEVICE.
     if (wSetup.DVB_Type < SCAN_NO_DEVICE)
        ProcessKey(kRight);
     }
  else if (! wSetup.systems[wSetup.DVB_Type]) {
     if (direction) {
        if (wSetup.DVB_Type == SCAN_NO_DEVICE) {
           wSetup.DVB_Type = 1;
           ProcessKey(kLeft); // now, DVB_Type is 0.
           }
        else
           ProcessKey(kRight);
        }
     else {
        if (wSetup.DVB_Type == 0)
           wSetup.DVB_Type = SCAN_NO_DEVICE;
        ProcessKey(kLeft);
        }
     }

  return state;      
}



/*******************************************************************************
 * class cMenuScanning
 ******************************************************************************/
cMenuScanning::cMenuScanning(void) :
  needs_update(true), log_busy(false), transponder(0), transponders(1) {
  SetHelp(tr("Stop"), tr("Start"), tr("Settings"), "");

  wSetup.InitSystems();

  if (not ScanAvailable()) {
     AddCategory("NO DEVICES FOUND.");
     return;
     }

  std::string status(DVB_Types[wSetup.DVB_Type]);
  status += " ";
  if (wSetup.DVB_Type == SCAN_SATELLITE)
     status += sat_list[wSetup.SatIndex].full_name;
  else
     status += country_list[wSetup.CountryIndex].full_name;

  AddCategory(tr("Status"));
  Add((ScanType        = new cOsdItem(status.c_str()   )));
  Add((DeviceUsed      = new cOsdItem("Device:"        )));
  Add((Progress        = new cOsdItem("Scan:"          )));
  Add((CurrTransponder = new cOsdItem(" "              )));
  Add((Str             = new cOsdItem("STR"            )));

  AddCategory(tr("Channels"));
  Add((ChanAdd         = new cOsdItem(" "              )));
  Add((ChanNew         = new cOsdItem("known Channels:")));

  AddCategory(tr("Log Messages"));
  for(int i=0; i<LOGLEN; i++)
     Add((LogMsg[i] = new cOsdItem(" ")));

  SetChanAdd(wSetup.scanflags);
  MenuScanning = this;
}


cMenuScanning::~cMenuScanning(void) {
  MenuScanning = nullptr;
}


void cMenuScanning::SetChanAdd(size_t flags) {
  int lo =  flags & 3;
  int hi = (flags & 12) >> 2;

  std::string s = flagslo[lo] + " (" + flagshi[hi] + ")";
  ChanAdd->SetText(s.c_str(), true);
  ChanAdd->Set();
  if (MenuScanning)
     MenuScanning->Display();
}


void cMenuScanning::SetStatus(size_t status) {
  int type = Scanner?Scanner->DvbType() : wSetup.DVB_Type;
  static std::string s;

  s = DVB_Types[type];
  s += " ";
  if (type == SCAN_SATELLITE)
     s += sat_list[wSetup.SatIndex].full_name;
  else
     s += country_list[wSetup.CountryIndex].full_name;
  s += " ";
  s += st[std::min(status, st.size()-1)];

  ScanType->SetText(s.c_str(), true);
  ScanType->Set();
  MenuScanning->Display();
  lStatus = status;
}


void cMenuScanning::SetCounters(int curr_tp, int all_tp) {
  transponder = curr_tp;
  transponders = all_tp;
}


std::string cMenuScanning::TimeStr(void) {
  time_t t = time(0) - timestamp;
  return IntToStr(t/60) + 'm' + IntToStr(t%60) + 's';
}


void cMenuScanning::SetProgress(size_t progress) {
  if (transponder <= 0)
     progress = lProgress;

  std::string s = "Scan: " + IntToStr(progress) + "% running " +
                  TimeStr();

  if ((transponder > 0) and (transponders > 0)) {
     s += " (" + IntToStr(transponder) + '/' + IntToStr(transponders) + ')';
     lProgress = (int) (0.5 + (100.0 * transponder) / transponders);
     }

  Progress->SetText(s.c_str(), true);
  Progress->Set();
  if (needs_update) {
     SetStatus(lStatus);
     SetDeviceInfo(deviceName, false);
     needs_update = false;
     }
  MenuScanning->Display();
}


void cMenuScanning::SetTransponder(const TChannel* transponder) {
  std::string s;
  ((TChannel*) transponder)->PrintTransponder(s);
  CurrTransponder->SetText(s.c_str(), true);
  CurrTransponder->Set();
  MenuScanning->Display();
}


void cMenuScanning::SetStr(size_t strength, bool locked) {
  std::string s;

  if (locked and (strength == 0))
     strength = 100;
  else
     strength = std::min(strength, (size_t)100);

  std::string t = IntToStr(strength);
  s = "STR " + t + '%' + std::string(4-t.size(), ' ');

  std::string u((size_t)(0.5 + strength/12.5), '_');
  s+= '[' + u + ']' + std::string(8-u.size(), ' ');

  if (locked)
     s += "LOCK";

  Str->SetText(s.c_str(), true);
  Str->Set();
  MenuScanning->Display();
}


void cMenuScanning::SetChan(size_t count) {
  static std::string s;
  s = "known Channels: " + IntToStr(channelcount = count);

  ChanNew->SetText(s.c_str(), true);
  ChanNew->Set();
  MenuScanning->Display();
}


void cMenuScanning::SetDeviceInfo(std::string Info, bool update) {
  std::string s("Device ");
  if (update)
     deviceName = Info;

  s += deviceName;
  DeviceUsed->SetText(s.c_str(), true);
  DeviceUsed->Set();
  MenuScanning->Display();
}


void cMenuScanning::AddLogMsg(std::string Msg) {
  if (log_busy) return;
  log_busy = true;
  for(int i=0; i<LOGLEN-1; i++) {
     LogMsg[i]->SetText(LogMsg[i+1]->Text(), true);    
     LogMsg[i]->Set();
     }
  LogMsg[LOGLEN - 1]->SetText(Msg.c_str(), true);
  LogMsg[LOGLEN - 1]->Set();
  MenuScanning->Display();
  log_busy = false;
}


void cMenuScanning::AddCategory(std::string category) {
  Add(new cOsdItem(("---------------  " + category).c_str()));
}


eOSState cMenuScanning::ProcessKey(eKeys Key) {
  if (wSetup.update) {
     SetStatus(4);
     SetChanAdd(wSetup.scanflags);
     wSetup.update = false;
     }
  eOSState state = cMenuSetupPage::ProcessKey(Key);
  switch (Key) {
     case kUp:
     case kDown:
        return osContinue;
     default:;
     }
  if (state == osUnknown) {
     switch(Key) {
        case kBack:
        case kOk:
           state=osBack;
           return state;

        case kGreen:
           if (ScanAvailable()) {
              state=osContinue;
              needs_update = true;
              StartScan();
              }
           break;

        case kRed:
           if (ScanAvailable()) {
              state=osContinue;
              needs_update = true;
              StopScan();
              }
           break;

        case kYellow:
           if (ScanAvailable())
              return AddSubMenu(new cMenuSettings());
        default:
           break;
        }
    }
  if (Scanner && Scanner->Active() && (state != osBack))
     return osContinue;
  return state;      
}


bool cMenuScanning::StopScan(void) {
  DoStop();
  return true;
}


bool cMenuScanning::StartScan(void) {
  int type = wSetup.DVB_Type;
  dlog(0, "StartScan(" + std::string(DVB_Types[type]) + ')');

  if (!wSetup.systems[type]) {
     dlog(0, "Skipping scan: CANNOT SCAN - No device!");
     Skins.Message(mtInfo, tr("CANNOT SCAN - No device!"));
     mSleep(6000);
     return false;
     }

  return DoScan(type);
}


void cMenuScanning::Store(void) {
  thisPlugin->StoreSetup();
}



/*******************************************************************************
 * Stop Scanner now, we're destroying the plugin..
 ******************************************************************************/
void stopScanners(void) {
 if (Scanner) {
    dlog(0, "Stopping scanner.");
    Scanner->SetShouldstop(true);
    }
}


/*******************************************************************************
 * create new scanner.
 ******************************************************************************/
bool DoScan(int DVB_Type) {
  if (Scanner && Scanner->Active()) {
     dlog(0, "ERROR: already scanning");
     return false;
     }
  wSetup.InitSystems();
  if (DVB_Type == SCAN_NO_DEVICE || ! wSetup.systems[DVB_Type]) {
     dlog(0, "ERROR: no device found");
     return false;
     }
  timestamp = time(0);
  channelcount = 0;
  Scanner = new cScanner("wirbelscan Scanner", DVB_Type);
  return true;
}


/*******************************************************************************
 * Stop Scanner.
 ******************************************************************************/
void DoStop(void) {
  if (Scanner && Scanner->Active())
     Scanner->SetShouldstop(true);
}
