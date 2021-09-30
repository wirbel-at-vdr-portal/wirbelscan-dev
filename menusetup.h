/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <string>
#include <vdr/menuitems.h>

/*******************************************************************************
 * forward decls.
 ******************************************************************************/
class TChannel;


/*******************************************************************************
 * class cMenuScanning
 ******************************************************************************/
class cMenuScanning : public cMenuSetupPage {
private:
  bool needs_update;
  bool log_busy;
  int transponder;
  int transponders;
protected:
  virtual bool StartScan(void);
  virtual bool StopScan(void);
  virtual void AddCategory(std::string category);
public:
  cMenuScanning(void);
  ~cMenuScanning(void);
  virtual void Store(void);
  virtual eOSState ProcessKey(eKeys Key);
  void SetStatus(int status);
  void SetProgress(const int progress);
  void SetCounters(int curr_tp, int all_tp);
  void SetTransponder(const TChannel* transponder); 
  void SetStr(unsigned strength, bool locked);
  void SetChan(int count);
  void SetDeviceInfo(std::string Info, bool update = true);
  void SetChanAdd(uint32_t flags);
  void AddLogMsg(std::string Msg);
};



extern cMenuScanning* MenuScanning;
extern std::string deviceName;
extern std::string lTransponder;
extern int lProgress;
extern int lStrength;

void stopScanners(void);
bool DoScan(int DVB_Type);
void DoStop(void);
