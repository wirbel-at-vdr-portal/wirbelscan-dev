/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <vdr/thread.h> // cThread

class cDevice;
class cDvbDevice;
class TChannel;
class cStateMachine;

class cScanner : public cThread {
private:
  bool       shouldstop;
  bool       single;
  size_t     user[3];
  int        status;
  int        initialTransponders;
  int        newTransponders;
  int        thisChannel;
  int        type;
  cDevice*   dev;
  TChannel*  aChannel;
  cStateMachine* StateMachine;
protected:
  virtual void Action(void);
  void AddChannels(void);
public:
  cScanner(const char* Description, int Type);
  virtual ~cScanner(void);
  virtual void SetShouldstop(bool On);
  virtual bool ActionAllowed(void);
  int Status(void)  { return status; };
  int DvbType(void) { return type; };
  int InitialTransponders(void)  { return initialTransponders; };
  int ThisChannel(void)  { return thisChannel; };
  void Progress(void);
  cDvbDevice* DvbDevice(void);
};
