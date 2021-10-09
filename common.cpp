/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#include <thread>               // std::this_thread
#include <string>
#include <iostream>
#include <iomanip>              // std::setfill,std::setw
#include <algorithm>            // std::min
#include <sstream>              // std::stringstream
#include <chrono>               // std::chrono::milliseconds
#include <ctime>                // time_t, strftime
#include <cctype>               // std::isprint
#include <syslog.h>             // syslog()
#include <sys/stat.h>           // stat()
#include <linux/dvb/frontend.h> // fe_status_t, dvb_frontend_info
#include <linux/dvb/version.h>  // DVB_API_VERSION, DVB_API_VERSION_MINOR
#include <vdr/device.h>         // cDevice
#include <vdr/dvbdevice.h>      // cDvbDevice
#include <sys/ioctl.h>          // ioctl()
#include "common.h"             // 
#include "menusetup.h"          // MenuScanning
#include "satellites.h"         // txt_to_satellite()
#include "countries.h"          // txt_to_country()

/*******************************************************************************
 *  Generic functions which will be used in the whole plugin.
 ******************************************************************************/



/*******************************************************************************
 * class cMySetup, plugin setup data
 ******************************************************************************/
cMySetup::cMySetup(void) {
  verbosity            = 3;              /* default to messages           */
  DVB_Type             = SCAN_TERRESTRIAL;
  DVBT_Inversion       = 0;              /* auto/off                      */
  DVBC_Inversion       = 0;              /* auto/off                      */
  DVBC_Symbolrate      = 0;              /* default to AUTO               */
  DVBC_QAM             = 0;              /* default to AUTO               */
  DVBC_Network_PID     = 0x10;           /* as 300486                     */
  CountryIndex         = COUNTRY::txt_to_country("DE");
  SatIndex             = txt_to_satellite("S19E2");
  enable_s2            = 1;
  ATSC_type            = 0;              /* VSB                           */
  logFile              = STDOUT;         /* log errors/messages to stdout */
  scanflags            = SCAN_TV | SCAN_RADIO | SCAN_FTA | SCAN_SCRAMBLED;
  update               = false;
  initsystems          = false;
  scan_remove_invalid  = false;
  scan_update_existing = false;
  scan_append_new      = true;
}

void cMySetup::InitSystems(void) {
  memset(&systems[0], 0, sizeof(systems));
  while(! cDevice::WaitForAllDevicesReady(20)) mSleep(100);

  for(int i=0; i<cDevice::NumDevices(); i++) {
     cDevice* device = cDevice::GetDevice(i);
     if (device == nullptr)
        continue;

     if (device->ProvidesSource(cSource::FromString("A"))) systems[SCAN_TERRCABLE_ATSC] = 1;
     if (device->ProvidesSource(cSource::FromString("C"))) systems[SCAN_CABLE]          = 1;
     if (device->ProvidesSource(cSource::FromString("S"))) systems[SCAN_SATELLITE]      = 1;
     if (device->ProvidesSource(cSource::FromString("T"))) systems[SCAN_TERRESTRIAL]    = 1;
     }

  if (DVB_Type >= SCAN_NO_DEVICE || ! systems[DVB_Type]) {
     for(DVB_Type = SCAN_TERRESTRIAL; DVB_Type < SCAN_NO_DEVICE; DVB_Type++) {
        if (systems[DVB_Type])
           break;
        }
     }
  initsystems = true;
}

cMySetup wSetup;


/*******************************************************************************
 * plugins logging facility: dlog(), _log() and hexdump()
 ******************************************************************************/
void _log(const char* function, int line, const int level, std::string msg) {
  if (level > wSetup.verbosity)
     return;

  auto now = []()->std::string {
    char s[16];
    time_t t = time(nullptr);
    strftime(s, sizeof(s), "%H:%M:%S ", localtime(&t));
    return std::string(s);
    };

  if ((wSetup.logFile < STDOUT) or (wSetup.logFile > STDERR)) {
     syslog(LOG_DEBUG, "%s", "wirbelscan - WARNING: setting logFile to STDOUT");
     wSetup.logFile = STDOUT;
     }

  if (wSetup.logFile == SYSLOG)
     syslog(LOG_DEBUG, "%s", msg.c_str());
  else if (wSetup.logFile == STDOUT) {
     std::cout << now();
     if (wSetup.verbosity >= 5)
        std::cout << function << ':' << IntToStr(line) << ' ';
     std::cout << msg << std::endl;
     std::cout.flush();
     }
  else if (wSetup.logFile == STDERR) {
     std::cerr << now();
     if (wSetup.verbosity >= 5)
        std::cerr << function << ':' << IntToStr(line) << ' ';
     std::cerr << msg << std::endl;
     std::cerr.flush();
     }
  
  if (MenuScanning)
     MenuScanning->AddLogMsg(msg);
}

void hexdump(std::string intro, const unsigned char* buf, size_t len) {
  if (wSetup.verbosity < 3)
    return;

  std::string hex(size_t n, size_t digits);
  std::stringstream ss;
  std::string s;
  size_t addr_len = hex(len,2).size();

  if (buf == nullptr)
     len = 0;

  if (intro.size() < 48)
     s = std::string(48 - intro.size(), '=');

  ss << "\t===================== " << intro << " " << s << std::endl
     << "\tlen = " << len << std::endl;

  for(size_t i=0; i<len; i++) {
     size_t r = i % 16;
     unsigned char c = *(buf + i);

     if (r == 0) {
        if (i) ss << "\n";
        ss << "\t" << hex((i/16)*16, addr_len) << ": ";
        s = "                ";
        }

     ss << hex(c, 2) << ' ';
     if (std::isprint(c) == 0)
        c = 32;
     s[i % 16] = c;
     if (r == 15)
        ss << "; " << s;
     if (i == len-1) {
        size_t n = len % 16;
        if (n > 0)
           ss << std::string((16-n)*3, ' ') << "; " << s;
        }
     }

  ss << std::endl << "\t" << std::string(71, '=') << std::endl;

  std::cerr << ss.str() << std::endl;
}


/*******************************************************************************
 * encapsulation of the ioctl() syscall.
 ******************************************************************************/
int IOCTL(int fd, int cmd, void* data) {  
  for(int retry=10; retry>=0;) {
     if (ioctl(fd, cmd, data) != 0) {
        /* :-( */
        if (retry) {
           mSleep(10); /* 10msec */
           retry--;
           continue;
           }
        return -1;       /* :'-((  */
        }
     else
        return 0;        /* :-)    */
     }
  return 0;
}


bool FileExists(std::string aFile) {
  struct stat Stat;
  return stat(aFile.c_str(), &Stat) == 0;
}


int dvbc_modulation(int index) {
  switch(index) {
     case 0:   return 256;
     case 1:   return 64;
     case 2:   return 128;
     default:  return 999;
     }
}

int dvbc_symbolrate(int index) {
  switch(index) {
     case 0:   return 6900000;
     case 1:   return 6875000;
     case 2:   return 6111000;
     case 3:   return 6250000;
     case 4:   return 6790000;
     case 5:   return 6811000;
     case 6:   return 5900000;
     case 7:   return 5000000;
     case 8:   return 3450000;
     case 9:   return 4000000;
     case 10:  return 6950000;
     case 11:  return 7000000;
     case 12:  return 6952000;
     case 13:  return 5156000;
     case 14:  return 5483000;
     default:  return 0;
     }
}

/*******************************************************************************
 * TParams, read VDR param string and divide to separate items or vice versa.
 ******************************************************************************/

TParams::TParams() :
  Bandwidth(8), FEC(999), FEC_low(999), Guard(999), Polarization(0),
  Inversion(999), Modulation(2), Pilot(999), Rolloff(999),
  StreamId(0), SystemId(0), DelSys(0), Transmission(999),
  MISO(0), Hierarchy(999)
{}

TParams::TParams(std::string& s) :
  Bandwidth(8), FEC(999), FEC_low(999), Guard(999), Polarization(0),
  Inversion(999), Modulation(2), Pilot(999), Rolloff(999),
  StreamId(0), SystemId(0), DelSys(0), Transmission(999),
  MISO(0), Hierarchy(999)
{
  Parse(s);
}

void TParams::Parse(std::string& s) {
  std::transform(s.begin(), s.end() ,s.begin(), ::toupper);
  const char* c = s.c_str();
  while(*c)
     switch(*c) {
        case 'H': Polarization = *c++;     break; 
        case 'V': Polarization = *c++;     break;
        case 'L': Polarization = *c++;     break;
        case 'R': Polarization = *c++;     break;
        case 'B': Bandwidth    = Value(c); break;
        case 'C': FEC          = Value(c); break;
        case 'D': FEC_low      = Value(c); break;
        case 'G': Guard        = Value(c); break;
        case 'I': Inversion    = Value(c); break;
        case 'M': Modulation   = Value(c); break;
        case 'N': Pilot        = Value(c); break;
        case 'O': Rolloff      = Value(c); break;
        case 'P': StreamId     = Value(c); break;
        case 'Q': SystemId     = Value(c); break;
        case 'S': DelSys       = Value(c); break;
        case 'T': Transmission = Value(c); break;
        case 'X': MISO         = Value(c); break;
        case 'Y': Hierarchy    = Value(c); break;
        default : dlog(0, "error in '" + s + "': invalid char '" + *c + "'");
        }
}

int TParams::Value(const char*& s) {
  int v = 0;
  s++;
  while(*s) {
     switch(*s) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
           v = 10 * v + (*s++ - '0');
           break;
        default:
           return v;
        }
     }
  return v;
}

void TParams::Print(std::string& dest, char Source) {
  dest.clear();
  dest.reserve(18 * 4);

  switch(Source) {
     case 'A':
        if (Inversion != 999)       dest += "I" + IntToStr(Inversion);
        if (Modulation != 999)      dest += "M" + IntToStr(Modulation);
        break;
     case 'C':
        if (FEC != 999)             dest += "C" + IntToStr(FEC);
        if (Inversion != 999)       dest += "I" + IntToStr(Inversion);
        if (Modulation != 999)      dest += "M" + IntToStr(Modulation);
        break;
     case 'S':
        if (Polarization)           dest += Polarization;
        if (FEC != 999)             dest += "C" + IntToStr(FEC);
        if (Inversion != 999)       dest += "I" + IntToStr(Inversion);
        if (Modulation != 999)      dest += "M" + IntToStr(Modulation);
        if (DelSys) {
           if (Pilot != 999)        dest += "N" + IntToStr(Pilot);
           if (Rolloff != 999)      dest += "O" + IntToStr(Rolloff);
           if (StreamId != 999)     dest += "P" + IntToStr(StreamId);
           }
        if (DelSys != 999)          dest += "S" + IntToStr(DelSys);
        break;
     case 'T':
        if (Bandwidth != 999)       dest += "B" + IntToStr(Bandwidth);
        if (FEC != 999)             dest += "C" + IntToStr(FEC);
        if (FEC_low != 999)         dest += "D" + IntToStr(FEC_low);
        if (Guard != 999)           dest += "G" + IntToStr(Guard);
        if (Inversion != 999)       dest += "I" + IntToStr(Inversion);
        if (Modulation != 999)      dest += "M" + IntToStr(Modulation);
        if (DelSys) {
           if (StreamId != 999)     dest += "P" + IntToStr(StreamId);
           if (SystemId != 999)     dest += "Q" + IntToStr(SystemId);
           }
        if (DelSys != 999)          dest += "S" + IntToStr(DelSys);
        if (Transmission != 999)    dest += "T" + IntToStr(Transmission);
        if (DelSys and MISO != 999) dest += "X" + IntToStr(MISO);
        if (Hierarchy != 999)       dest += "Y" + IntToStr(Hierarchy);
        break;
     default: dlog(0, ": unknown Source " + IntToHex((size_t)Source, 2));
     }
}


/*******************************************************************************
 * class TChannel, internal channel representation.
 ******************************************************************************/
TChannel::TChannel() :
     Name("???"), Shortname(""), Provider(""), Frequency(0),
     Bandwidth(8), FEC(999), FEC_low(999), Guard(999), Polarization(0),
     Inversion(999), Modulation(2), Pilot(999), Rolloff(999),
     StreamId(0), SystemId(0), DelSys(0), Transmission(999),
     MISO(0), Hierarchy(999), Symbolrate(0), PCR(0), TPID(0),
     SID(0), ONID(0), NID(0), TID(0), RID(0), free_CA_mode(0),
     service_type(0xFFFF), OrbitalPos(0),
     reported(false), Tunable(false), Tested(false)
{}


TChannel& TChannel::operator= (const cChannel* rhs) {
  Name       = rhs->Name();
  Shortname  = rhs->ShortName();
  Provider   = rhs->Provider();
  Frequency  = rhs->Frequency();
  Source     = *cSource::ToString(rhs->Source());
  Symbolrate = rhs->Srate();
  VPID.PID   = rhs->Vpid();
  VPID.Type  = rhs->Vtype();
  PCR        = rhs->Ppid();
  TPID       = rhs->Tpid();
  SID        = rhs->Sid();
  NID = ONID = rhs->Nid();
  TID        = rhs->Tid();
  RID        = rhs->Rid();

  APIDs.Clear();
  for(int i = 0; i < MAXAPIDS and rhs->Apid(i); ++i) {
     TPid a;
     a.PID = rhs->Apid(i);
     a.Type = rhs->Atype(i);
     a.Lang = rhs->Alang(i);
     APIDs.Add(a);
     }

  DPIDs.Clear();
  for(int i = 0; i < MAXDPIDS and rhs->Dpid(i); ++i) {
     TPid d;
     d.PID = rhs->Dpid(i);
     d.Type = rhs->Dtype(i);
     d.Lang = rhs->Dlang(i);
     DPIDs.Add(d);
     }

  SPIDs.Clear();
  for(int i = 0; i < MAXSPIDS and rhs->Spid(i); ++i) {
     TPid s;
     s.PID = rhs->Spid(i);
     s.Type = 0;
     s.Lang = rhs->Slang(i);
     SPIDs.Add(s);
     }

  CAIDs.Clear();
  for(int i = 0; i < MAXCAIDS and rhs->Ca(i); ++i) {
     int ca = rhs->Ca(i);
     CAIDs.Add(ca);
     }

  std::string parameters = rhs->Parameters();

  TParams p(parameters);
  Bandwidth    = p.Bandwidth;
  FEC          = p.FEC;
  FEC_low      = p.FEC_low;
  Guard        = p.Guard;
  Polarization = p.Polarization;
  Inversion    = p.Inversion;
  Modulation   = p.Modulation;
  Pilot        = p.Pilot;
  Rolloff      = p.Rolloff;
  StreamId     = p.StreamId;
  SystemId     = p.SystemId;
  DelSys       = p.DelSys;
  Transmission = p.Transmission;
  MISO         = p.MISO;
  Hierarchy    = p.Hierarchy;
  return *this;
}

void TChannel::CopyTransponderData(const TChannel* Channel) {
  if (Channel) {
     Frequency    = Channel->Frequency;
     Source       = Channel->Source;
     Symbolrate   = Channel->Symbolrate;
     Bandwidth    = Channel->Bandwidth;
     FEC          = Channel->FEC;
     FEC_low      = Channel->FEC_low;
     Guard        = Channel->Guard;
     Polarization = Channel->Polarization;
     Inversion    = Channel->Inversion;
     Modulation   = Channel->Modulation;
     Pilot        = Channel->Pilot;
     Rolloff      = Channel->Rolloff;
     StreamId     = Channel->StreamId;
     SystemId     = Channel->SystemId;
     DelSys       = Channel->DelSys;
     Transmission = Channel->Transmission;
     MISO         = Channel->MISO;
     Hierarchy    = Channel->Hierarchy;
     }
}

void TChannel::Params(std::string& s) {
  s.clear();
  s.reserve(18 * 4);

  if (Source.size() == 0)
     return;

  switch(Source[0]) {
     case 'A':
        if (Inversion != 999)       s += "I" + IntToStr(Inversion);
        if (Modulation != 999)      s += "M" + IntToStr(Modulation);
        break;
     case 'C':
        if (FEC != 999)             s += "C" + IntToStr(FEC);
        if (Inversion != 999)       s += "I" + IntToStr(Inversion);
        if (Modulation != 999)      s += "M" + IntToStr(Modulation);
        break;
     case 'S':
        if (Polarization)           s += Polarization;
        if (FEC != 999)             s += "C" + IntToStr(FEC);
        if (Inversion != 999)       s += "I" + IntToStr(Inversion);
        if (Modulation != 999)      s += "M" + IntToStr(Modulation);
        if (DelSys) {
           if (Pilot != 999)        s += "N" + IntToStr(Pilot);
           if (Rolloff != 999)      s += "O" + IntToStr(Rolloff);
           if (StreamId != 999)     s += "P" + IntToStr(StreamId);
           }
        if (DelSys != 999)          s += "S" + IntToStr(DelSys);
        break;
     case 'T':
        if (Bandwidth != 999)       s += "B" + IntToStr(Bandwidth);
        if (FEC != 999)             s += "C" + IntToStr(FEC);
        if (FEC_low != 999)         s += "D" + IntToStr(FEC_low);
        if (Guard != 999)           s += "G" + IntToStr(Guard);
        if (Inversion != 999)       s += "I" + IntToStr(Inversion);
        if (Modulation != 999)      s += "M" + IntToStr(Modulation);
        if (DelSys) {
           if (StreamId != 999)     s += "P" + IntToStr(StreamId);
           if (SystemId != 999)     s += "Q" + IntToStr(SystemId);
           }
        if (DelSys != 999)          s += "S" + IntToStr(DelSys);
        if (Transmission != 999)    s += "T" + IntToStr(Transmission);
        if (DelSys and MISO != 999) s += "X" + IntToStr(MISO);
        if (Hierarchy != 999)       s += "Y" + IntToStr(Hierarchy);
        break;
     default: dlog(0, ": unknown Source " + Source);
     }
}

void TChannel::PrintTransponder(std::string& dest) {
  std::stringstream ss;
  std::string params;
  Params(params);
  int i = Frequency;
  char source = Source[0];

  if (i < 1000)    i *= 1000;
  if (i > 999999)  i /= 1000;

  ss << source;

  if (DelSys == 1)
     ss << "2 ";
  else
     ss << "  ";

  ss << FloatToStr((source == 'S')?i:i/1000.0, 8, 2) << " MHz";

  if ((source == 'C') or (source == 'S')) {
     i = Symbolrate;
     if (i < 1000)    i *= 1000;
     if (i > 999999)  i /= 1000;
     ss << " SR " << IntToStr(i) << ' ' << params;
     }

  dest = std::move(ss.str());
}

void TChannel::Print(std::string& dest) {
  std::stringstream ss;
  std::string params;
  Params(params);

  if (Name.empty())
     ss << "NULL";
  else
     ss << Name;

  if (not Shortname.empty())
     ss <<  ',' << Shortname;

  if (not Provider.empty())
     ss <<  ';' << Provider;

  ss << ':' << IntToStr(Frequency)
     << ':' << params
     << ':' << Source 
     << ':' << IntToStr(Symbolrate)
     << ':' << IntToStr(VPID.PID);

  if (PCR and (PCR != VPID.PID))
     ss << '+' << IntToStr(PCR);

  if (VPID.Type)
     ss << '=' << IntToStr(VPID.Type);

  if (APIDs.Count()) {
     for(int i=0; i<APIDs.Count(); ++i) {
        if (i == 0)
           ss << ':';
        else
           ss << ',';
        ss << IntToStr(APIDs[i].PID);
        if (not APIDs[i].Lang.empty())
           ss << '=' << APIDs[i].Lang;
        if (APIDs[i].Type)
           ss << '@' << IntToStr(APIDs[i].Type);
        }
     }
  else
     ss << ":0";

  if (DPIDs.Count()) {
     for(int i=0; i<DPIDs.Count(); ++i) {
        if (i == 0)
           ss << ';';
        else
           ss << ',';
        ss << IntToStr(DPIDs[i].PID);
        if (not DPIDs[i].Lang.empty())
           ss << '=' << DPIDs[i].Lang;
        if (DPIDs[i].Type)
           ss << '@' << IntToStr(DPIDs[i].Type);
        }
     }
  ss << ':' << IntToStr(TPID);
  if (CAIDs.Count()) {
     for(int i=0; i<CAIDs.Count(); ++i) {
        if (i == 0)
           ss << ':';
        else
           ss << ',';
        ss << std::nouppercase << std::hex << CAIDs[i] << std::dec;
        }
     }
  else
     ss << ":0";
  ss << ':' << IntToStr(SID)
     << ':' << IntToStr(ONID)
     << ':' << IntToStr(TID)
     << ':' << IntToStr(RID);

  dest = std::move(ss.str());
}

void TChannel::VdrChannel(cChannel& c) {
  std::string s;
  Print(s);
  c.Parse(s.c_str());
}

static bool SourceMatches(int a, int b) {
  static const int SatRotor = cSource::stSat | cSource::st_Any;
  return (a == b or (a == SatRotor and (b & cSource::stSat)));
}

bool TChannel::ValidSatIf() {
  int f = Frequency;

  while(f > 999999) f /= 1000;

  if (Setup.DiSEqC) {
     cDiseqc* d;
     for(d = Diseqcs.First(); d; d = Diseqcs.Next(d))
        if (SourceMatches(d->Source(), cSource::FromString(Source.c_str())) and
            d->Slof() > f and d->Polarization() == Polarization) {
           f -= d->Lof();
           break;
           }
     if (!d) {
        dlog(0, "no diseqc settings for (" +
                Source + ", " + IntToStr(Frequency) + ", " + Polarization + ')');
        return false;
        }
     }
  else
     f -= f < Setup.LnbSLOF ? Setup.LnbFrequLo : Setup.LnbFrequHi;

  if (f < 950 or f > 2150) {
     dlog(0, "transponder (" +
             Source + ", " + IntToStr(Frequency) + ", " + Polarization +
             ") (freq " + IntToStr(f) + " -> out of tuning range)");
     return false;
     }
  return true;
}

void mSleep(size_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::string IntToStr(int n, size_t digits) {
  std::stringstream ss;
  if (n < 0) {
     n *= -1;
     ss << '-';
     digits = std::min(digits,digits-1);
     }
  ss << std::setfill('0') << std::setw(digits) << n;
  return ss.str();
}

std::string hex(size_t n, size_t digits) {
  std::stringstream ss;
  ss << std::uppercase << std::setfill('0') << std::hex << std::setw(digits) << n;
  return ss.str();
}

std::string IntToHex(size_t n, size_t digits) {
  return "0x" + hex(n, digits);
}
  
std::string FloatToStr(double f, size_t width, size_t precision) {
  std::stringstream ss;
  ss.precision(precision);
  ss << std::fixed << std::setfill(' ') << std::setw(width) << f;
  return ss.str();
}

cDvbDevice* GetDvbDevice(cDevice* d) {
  #ifdef __DYNAMIC_DEVICE_PROBE
     /* vdr/device.h was patched for dynamite plugin */
     if (d == nullptr)
        return nullptr;
     if (d->HasSubDevice())
        return dynamic_cast<cDvbDevice*>(d->SubDevice());
  #endif

  return dynamic_cast<cDvbDevice*>(d);
}

void PrintDvbApi(std::string& s) {
  s = "compiled for DVB API "
      + IntToStr(DVB_API_VERSION)
      + '.'
      + IntToStr(DVB_API_VERSION_MINOR);
}

unsigned int GetFrontendStatus(cDevice* dev) {
  fe_status_t status = FE_NONE;  
  cDvbDevice* dvbdevice = GetDvbDevice(dev);
  if (dvbdevice == nullptr) return status; 

  std::string s = "/dev/dvb/adapter" + std::to_string(dvbdevice->Adapter()) +
                  "/frontend"        + std::to_string(dvbdevice->Frontend());

  int fe = open(s.c_str(), O_RDONLY | O_NONBLOCK);
  if (fe < 0)
     dlog(0, "could not open " + s);
  else {
     if (IOCTL(fe, FE_READ_STATUS, &status) < 0)
        dlog(0, "could not read status: " + s);
     close(fe);
     }
  return status;
}

unsigned int GetCapabilities(cDevice* dev) {
  struct dvb_frontend_info fe_info;
  fe_info.caps = FE_IS_STUPID;

  cDvbDevice* dvbdevice = GetDvbDevice(dev);
  if (dvbdevice == nullptr) return fe_info.caps;

  std::string s = "/dev/dvb/adapter" + std::to_string(dvbdevice->Adapter()) +
                  "/frontend"        + std::to_string(dvbdevice->Frontend());


  int fe = open(s.c_str(), O_RDONLY | O_NONBLOCK);
  if (fe < 0)
     dlog(0, "could not open " + s);
  else {
     if (IOCTL(fe, FE_GET_INFO, &fe_info) < 0)
        dlog(0, "could not query: " + s);
     close(fe);
     }
  return fe_info.caps;
}

bool GetTerrCapabilities(cDevice* dev, bool* CodeRate, bool* Modulation, bool* Inversion, bool* Bandwidth, bool* Hierarchy,
                          bool* TransmissionMode, bool* GuardInterval, bool* DvbT2) {
  unsigned int cap = GetCapabilities(dev);
  *CodeRate         = cap & FE_CAN_FEC_AUTO;
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *Inversion        = cap & FE_CAN_INVERSION_AUTO; 
  *Bandwidth        = cap & FE_CAN_BANDWIDTH_AUTO;
  *Hierarchy        = cap & FE_CAN_HIERARCHY_AUTO;
  *TransmissionMode = cap & FE_CAN_GUARD_INTERVAL_AUTO;
  *GuardInterval    = cap & FE_CAN_TRANSMISSION_MODE_AUTO;
  *DvbT2            = cap & FE_CAN_2G_MODULATION;
  return cap != FE_IS_STUPID;
}

bool GetCableCapabilities(cDevice* dev, bool* Modulation, bool* Inversion) {
  unsigned int cap = GetCapabilities(dev);
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *Inversion        = cap & FE_CAN_INVERSION_AUTO;
  return cap != FE_IS_STUPID;
}

bool GetAtscCapabilities(cDevice* dev, bool* Modulation, bool* Inversion, bool* VSB, bool* QAM) {
  unsigned int cap = GetCapabilities(dev);
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *Inversion        = cap & FE_CAN_INVERSION_AUTO;
  *VSB              = cap & FE_CAN_8VSB;
  *QAM              = cap & FE_CAN_QAM_256;
  return cap != FE_IS_STUPID;
}

bool GetSatCapabilities(cDevice* dev, bool* CodeRate, bool* Modulation, bool* RollOff, bool* DvbS2) {
  unsigned int cap = GetCapabilities(dev);
  *CodeRate         = cap & FE_CAN_FEC_AUTO;
  *Modulation       = cap & FE_CAN_QAM_AUTO;
  *RollOff          = 0; /* deprecated: bool* RollOff */
  *DvbS2            = cap & FE_CAN_2G_MODULATION;
  return cap != FE_IS_STUPID;
}
