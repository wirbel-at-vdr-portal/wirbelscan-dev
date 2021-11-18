/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <repfunc.h>

/*******************************************************************************
 * forward decls
 ******************************************************************************/
class cDevice;
class cDvbDevice;
class TChannel;


/*******************************************************************************
 * class cStateMachine
 ******************************************************************************/
class cStateMachine : public ThreadBase {
private:
  enum eState {
     eStart = 0,       // init, add next_from_list to NewTransponders      (NextTransponder)
     eStop,            // cleanup and leave loop                           ()
     eTune,            // tune, check for lock                             (AttachReceiver, NextTransponder)
     eNextTransponder, // next unsearched transponder from NewTransponders (Tune, Stop)
     eDetachReceiver,  // detach all receivers                             (NextTransponder)
     eScanPat,         // pat/pmt scan                                     (ScanNit)
     eScanPmt,
     eScanNit,         // nit scan                                         (ScanNitOther)
     eScanSdt,         // sdt scan                                         (ScanSdtOther)
     eScanEit,         // eit scan                                         (DetachReceiver)
     eUnknown,         // oops                                             (Stop)
     eAddChannels,     // adding results
     eGetTables,
     ePatNG,
     };
  eState      state, lastState;
  TChannel*   initial;
  cDevice*    dev;
  cDvbDevice* dvbdevice;
  bool        stop;
  bool        useNit;
  void*       parent;
protected:
  virtual void Action(void);
  virtual void Report(eState State);
public:
  cStateMachine(cDevice* Dev, TChannel* InitialTransponder, bool UseNit, void* Parent);
  virtual ~cStateMachine(void);
  void DoStop(void);
  bool Active(void);
};
