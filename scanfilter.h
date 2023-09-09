/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <string>
#include <cstdint>        // uint{8.16,32}_t
#include <atomic>         // std::atomic<bool>
#include <vdr/thread.h>   // cCondWait
#include <vdr/sections.h> // cSectionSyncer
#include "tlist.h"        // TList<T>
#include "common.h"       // TPid


/*******************************************************************************
 * forward decls
 ******************************************************************************/
class cDevice;
class TChannel;
extern int nextTransponders;

bool known_transponder(TChannel* newChannel, bool auto_allowed, TChannels* list = nullptr);
bool is_nearly_same_frequency(const TChannel* chan_a, const TChannel* chan_b, unsigned delta = 2001);
bool is_different_transponder_deep_scan(const TChannel* a, const TChannel* b, bool auto_allowed);
TChannel* GetByTransponder(const TChannel* Transponder);
void resetLists(void);


/*******************************************************************************
 * class cTransponders
 ******************************************************************************/
class cTransponders : public TChannels {
private:
protected:
public:
   bool IsUniqueTransponder(const TChannel* NewTransponder);
   TChannel* GetByParams(const TChannel* NewTransponder);
   TChannel* NextTransponder(void);
};



struct service {
  uint16_t transport_stream_id;
  uint16_t program_map_PID;
  uint16_t program_number;
};

struct TPatData {
  uint16_t network_PID;
  TList<struct service> services;
};

struct TPmtData {
  uint16_t program_map_PID;
  uint16_t program_number;
  uint16_t PCR_PID;

  TPid Vpid;
  int Tpid;
  TList<TPid> Apids;
  TList<TPid> Dpids;
  TList<TPid> Spids;
  TList<int> Caids;
};

struct TCell {
  uint16_t network_id;
  uint16_t cell_id;
  uint32_t frequency;
  uint8_t subcellcount;
  struct {
     uint8_t cell_id_extension;
     uint32_t transposer_frequency;
     } subcells[51];
};

struct TServiceListItem {
  uint16_t network_id;
  uint16_t original_network_id;
  uint16_t transport_stream_id;
  uint16_t service_id;
  uint16_t service_type;
};

struct TFrequencyListItem {
  uint16_t network_id;
  uint32_t frequency;
};

struct TChannelListItem {
  uint16_t network_id;
  uint16_t original_network_id;
  uint16_t transport_stream_id;
  uint16_t service_id;
  int channel_list_id;
  bool HD_simulcast;
  int LCN;
  int LCN_minor;
  bool operator <(const TChannelListItem& rhs);
  bool operator==(const TChannelListItem& rhs);
};

// returns true, if GetLCN() assigned a new LCN to 'c'.
bool GetLCN(TChannel* c);

struct TNitData {
  int OrbitalPos;
  bool West;
  TList<TFrequencyListItem> frequency_list;
  TList<TCell> cell_frequency_links;
  TList<TServiceListItem> service_types;
  TList<TChannel*> transport_streams;
};

struct sdtservice {
  uint16_t transport_stream_id;
  uint16_t original_network_id;
  uint16_t service_id;
  uint16_t service_type;
  bool free_CA_mode;
  std::string Name;
  std::string Shortname;
  std::string Provider;
  bool reported;
};

struct TSdtData {
  uint16_t original_network_id;
  TList<sdtservice> services;
};


/*******************************************************************************
 * class cPatScanner
 ******************************************************************************/
class cPatScanner : public ThreadBase {
private:
  cDevice* device;
  struct TPatData& PatData;
  std::atomic<bool> isActive;
  cSectionSyncer Sync;
  std::string s;
  cCondWait wait;
  TChannel channel;
  std::atomic<bool> hasPAT;
  bool anyBytes;
protected:
  virtual void Process(const unsigned char* Data, int Length);
  virtual void Action(void);
public:
  cPatScanner(cDevice* Parent, struct TPatData& Dest);
  ~cPatScanner();
  bool HasPAT(void) { return hasPAT; };
  bool Active(void) { return isActive; };
};


/*******************************************************************************
 * class cPmtScanner
 ******************************************************************************/
class cPmtScanner : public ThreadBase {
private:
  cDevice* device;
  TPmtData* data;
  bool isActive;
  bool jobDone;
  std::string s;
  cCondWait wait;
protected:
  virtual void Process(const unsigned char* Data, int Length);
  virtual void Action(void);
public:
  cPmtScanner(cDevice* Parent, TPmtData* Data);
  ~cPmtScanner();
  bool Active(void) { return isActive; };
  bool Finished(void) { return jobDone; };
};


/*******************************************************************************
 * class cNitScanner
 ******************************************************************************/
class cNitScanner : public ThreadBase {
private:
  bool active;
  cDevice* device;
  uint16_t nit;
  std::string s;
  cCondWait wait;
  TNitData& data;
  uint32_t first_crc32;
  int type;
  bool hasNIT;
  bool west;
  uint16_t orbital;
  bool anyBytes;
  void ParseCellFrequencyLinks(uint16_t network_id, const unsigned char* Data, TList<TCell>& list);
protected:
  virtual void Process(const unsigned char* Data, int Length);
  virtual void Action(void);
public:
  cNitScanner(cDevice* Parent, uint16_t network_PID, TNitData& Data, int Type);
  ~cNitScanner();
  bool Active(void) { return (active); };
  bool HasNIT(void) { return hasNIT; };
};


/*******************************************************************************
 * class cSdtScanner
 ******************************************************************************/
class cSdtScanner : public ThreadBase {
private:
  bool active;
  cDevice* device;
  TSdtData& data;
  std::string s;
  cCondWait wait;
  uint32_t first_crc32;
  bool hasSDT;
  bool anyBytes;
protected:
  virtual void Process(const unsigned char* Data, int Length);
  virtual void Action(void);
public:
  cSdtScanner(cDevice* Parent, TSdtData& Data);
  ~cSdtScanner();
  bool Active(void) { return active; };
  bool SdtNIT(void) { return hasSDT; };
};
