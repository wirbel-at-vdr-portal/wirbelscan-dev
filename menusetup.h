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
  std::string TimeStr(void);
protected:
  virtual bool StartScan(void);
  virtual bool StopScan(void);
  virtual void AddCategory(std::string category);
public:
  cMenuScanning(void);
  ~cMenuScanning(void);
  virtual void Store(void);
  virtual eOSState ProcessKey(eKeys Key);
  void SetStatus(size_t status);
  void SetProgress(size_t progress);
  void SetCounters(int curr_tp, int all_tp);
  void SetTransponder(const TChannel* transponder); 
  void SetStr(size_t strength, bool locked);
  void SetChan(size_t count);
  void SetDeviceName(std::string Name, bool update = true);
  void SetChanAdd(size_t flags);
  void AddLogMsg(std::string Msg);
};



extern cMenuScanning* MenuScanning;
extern std::string lDeviceName;
extern std::string lTransponder;
extern size_t lProgress;
extern size_t lStrength;

void stopScanners(void);
bool DoScan(int DVB_Type);
void DoStop(void);
