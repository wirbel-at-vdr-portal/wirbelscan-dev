/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <string>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <vdr/diseqc.h>
#include "tlist.h"

#define SCAN_TERRESTRIAL        0 /* DVB-T/T2                */
#define SCAN_CABLE              1 /* DVB-C                   */
#define SCAN_SATELLITE          2 /* DVB-S/S2                */
#define SCAN_TERRCABLE_ATSC     5 /* ATSC VSB and/or QAM     */
#define SCAN_NO_DEVICE          6
#define SCAN_TRANSPONDER        999

#define SCAN_TV        ( 1 << 0 )
#define SCAN_RADIO     ( 1 << 1 )
#define SCAN_FTA       ( 1 << 2 )
#define SCAN_SCRAMBLED ( 1 << 3 )

#define ADAPTER_AUTO            0

#define DVBC_INVERSION_AUTO     0
#define DVBC_QAM_AUTO           0
#define DVBC_QAM_64             1
#define DVBC_QAM_128            2
#define DVBC_QAM_256            3

#define EAST_FLAG               0
#define WEST_FLAG               1

#define MAXSIGNALSTRENGTH       65535
#define MINSIGNALSTRENGTH       16383

#define STDOUT                  1
#define SYSLOG                  2
#define STDERR                  3

#define HEXDUMP(d, l) \
  if (wSetup.verbosity > 5) hexdump(__PRETTY_FUNCTION__, d, l);


#define dlog(level, fmt...) do { \
   _log(__PRETTY_FUNCTION__,__LINE__, level, true, fmt); \
   } while(0)

void _log(const char* function, int line, const int level, bool newline, const char* fmt, ...);

#define fatal(x...)     dlog(0, x); return -1
#define warning(msg...) _log(__FUNCTION__,__LINE__,1,false,msg)
#define info(msg...)    _log(__FUNCTION__,__LINE__,2,false,msg)
#define verbose(x...)   dlog(4, x)

/*******************************************************************************
 * forward decls.
 ******************************************************************************/
class cChannel;
class cDevice;
class cDvbDevice;


/*******************************************************************************
 * class TParams, provide VDR param string as separate items.
 ******************************************************************************/
class TParams {
private:
  int Value(const char*& s);
public:
  TParams();
  TParams(std::string& s);
  void Parse(std::string& s);
  void Print(std::string& dest, char Source);  // Source = {'A','C','S','T'}
public:
  int  Bandwidth;
  int  FEC;
  int  FEC_low;
  int  Guard;
  char Polarization;
  int  Inversion;
  int  Modulation;
  int  Pilot;
  int  Rolloff;
  int  StreamId;
  int  SystemId;
  int  DelSys;
  int  Transmission;
  int  MISO;
  int  Hierarchy;
};


/*******************************************************************************
 * TChannel, internal channel representation.
 ******************************************************************************/

class TPid {
public:
  TPid() : PID(0), Type(0), Lang("") {}
  int PID;
  int Type;
  std::string Lang;
};

struct transposer {
  uint8_t  cell_id_extension;
  uint32_t transposer_frequency;
};

struct cell {
  uint16_t cell_id;

  // if TFS: up to 6 RF freqs.
  int num_center_frequencies;
  uint32_t center_frequencies[6];

  int num_transposers;
  struct transposer transposers[16];
};

class TChannel {
public:
  std::string Name;        // ':' replaced by '|', may contain ','
  std::string Shortname;   // ',' replaced by '.'
  std::string Provider;
  int Frequency;           // S:MHz, C,T: MHz,kHz,Hz
  int Bandwidth;           // 'B' 1712, 5, 6, 7, 8, 10, DVB-T/DVB-T2 only
  int FEC;                 // 'C' 0, 12, 23, 34, 35, 45, 56, 67, 78, 89, 910
  int FEC_low;             // 'D', DVB-T/DVB-T2 only
  int Guard;               // 'G' 4, 8, 16, 32, 128, 19128, 19256: DVB-T/DVB-T2 only
  char Polarization;       // 'H', 'V', 'L', 'R'
  int Inversion;           // 'I' 0, 1 :  DVB-T and DVB-C only
  int Modulation;          // 'M' 2, 5, 6, 7, 10, 11, 12, 16, 32, 64, 128, 256, 999
                           //      2     QPSK (DVB-S, DVB-S2, DVB-T, DVB-T2, ISDB-T)
                           //      5     8PSK (DVB-S, DVB-S2)
                           //      6     16APSK (DVB-S2)
                           //      7     32APSK (DVB-S2)
                           //      10    VSB8 (ATSC aerial)
                           //      11    VSB16 (ATSC aerial)
                           //      12    DQPSK (ISDB-T)
                           //      16    QAM16 (DVB-T, DVB-T2, ISDB-T)
                           //      32    QAM32
                           //      64    QAM64 (DVB-C, DVB-T, DVB-T2, ISDB-T)
                           //      128   QAM128 (DVB-C)
                           //      256   QAM256 (DVB-C, DVB-T2)
  int Pilot;               // 'N' 0, 1, 999: DVB-S2 only
  int Rolloff;             // 'O' 0, 20, 25, 35
  int StreamId;            // 'P' 0-255
  int SystemId;            // 'Q' 0-65535
  int DelSys;              // 'S' 0, 1
  int Transmission;        // 'T' 1, 2, 4, 8, 16, 32: DVB-T/DVB-T2 only
  int MISO;                // 'X' 0 = siso, 1 = miso
  int Hierarchy;           // 'Y' 0, 1, 2, 4
  std::string Source;      // as defined in the file sources.conf
  int Symbolrate;          // DVB-S and DVB-C only
  //--
  TPid VPID;               // video PID, type may follow VPID, separated by '='
  int PCR;                 // may follow VPID, separated by '+'
  TList<TPid> APIDs;       // separated by commas: 101=deu@4. Ends with ';' if Dpids follow.
  TList<TPid> DPIDs;       // separated by commas: 103=deu@4
  int TPID;                // The teletext PID.  If this channel also carries DVB subtitles,
  TList<TPid> SPIDs;       //   the DVB subtitling PIDs follow the teletext PID, sep by a ';'
  TList<int> CAIDs;        // hex int (!) list.
  int SID;                 // Service ID
  int ONID;                // original Network ID
  int NID;                 // Network ID
  int TID;                 // Transport stream ID
  int RID;                 // just to mark invalid channels.
  int PMT;
  bool free_CA_mode;
  uint16_t service_type;
  int  OrbitalPos;
  bool West;
  bool reported;
  bool Tunable;
  bool Tested;
  TList<struct cell> cells;
public:
  TChannel(void);
  TChannel& operator= (const cChannel* rhs);
  void CopyTransponderData(const TChannel* Channel);
  void Params(std::string& s);
  void PrintTransponder(std::string& dest);
  void Print(std::string& dest);
  void VdrChannel(cChannel& c);
  bool ValidSatIf(void);
};

bool is_different_transponder_deep_scan(const TChannel* a, const TChannel* b, bool auto_allowed);

class TChannels : public TList<TChannel*> {
private:
public:
  TChannel* GetByParams(const TChannel* NewTransponder) {
     for(auto t:v) {
        if (!is_different_transponder_deep_scan(t, NewTransponder, true))
           return t;
        }
     return nullptr;
     }

  bool IsUniqueTransponder(const TChannel* NewTransponder) {
     return (GetByParams(NewTransponder) == nullptr);
     }
};


class cMySetup {
public:
  int verbosity;
  int logFile;
  int DVB_Type;
  int DVBT_Inversion;
  int DVBC_Inversion;
  int DVBC_Symbolrate;
  int DVBC_QAM;
  int DVBC_Network_PID;
  int CountryIndex;
  int SatIndex;
  int enable_s2;
  int ATSC_type;
  uint32_t scanflags;
  bool update;
  uint32_t user[3];
  int systems[8];
  bool initsystems;
  int scan_remove_invalid;
  int scan_update_existing;
  int scan_append_new;
public:
  cMySetup(void);
  void InitSystems(void);
};
extern cMySetup wSetup;


/* generic functions */
void hexdump(const char* intro, const unsigned char* buf, int len);
int  IOCTL(int fd, int cmd, void* data);
bool FileExists(const char* aFile);
void mSleep(size_t ms);

template<class T> inline void DeleteNullptr(T*& aClass) {
  T* temp = aClass;
  aClass = nullptr;
  delete temp;
}

template<class T> inline T BCDtoDecimal(T bcd) {
  T result = 0;
  for(T scale=1; bcd; scale*=10,bcd>>=4)
     result += scale * (bcd & 15);
  return result;
}

cDvbDevice* GetDvbDevice(cDevice* d);
int dvbc_modulation(int index);
int dvbc_symbolrate(int index);
void InitSystems(void);


void PrintDvbApi(std::string& s);
unsigned int GetFrontendStatus(cDevice* dev);
bool GetTerrCapabilities (cDevice* dev, bool* CodeRate, bool* Modulation, bool* Inversion, bool* Bandwidth, bool* Hierarchy, bool* TransmissionMode, bool* GuardInterval, bool* DvbT2);
bool GetCableCapabilities(cDevice* dev, bool* Modulation, bool* Inversion);
bool GetAtscCapabilities (cDevice* dev, bool* Modulation, bool* Inversion, bool* VSB, bool* QAM);
bool GetSatCapabilities  (cDevice* dev, bool* CodeRate, bool* Modulation, bool* RollOff, bool* DvbS2);
