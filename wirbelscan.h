/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <vdr/plugin.h>

class cPluginWirbelscan : public cPlugin {
private:
  int servicetype(const char* id, bool init = false);
public:
  cPluginWirbelscan(void);
  virtual ~cPluginWirbelscan();
  virtual const char* Version(void);
  virtual const char* Description(void);
  virtual const char* CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char* argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual const char* MainMenuEntry(void);
  virtual cOsdObject* MainMenuAction(void);
  virtual cMenuSetupPage* SetupMenu(void);
  virtual bool SetupParse(const char* Name, const char* Value);
  virtual void StoreSetup(void);
  virtual bool Service(const char* Id, void* Data = nullptr);
  virtual const char** SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char* Command, const char* Option, int& ReplyCode);
};
extern cPluginWirbelscan* thisPlugin;
