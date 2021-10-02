/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 * series  - A plugin for the Video Disk Recorder
 * wirbelscan - A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <thread>
#include <string>
#include <future>
#include <chrono>
#include <algorithm>
#include <utility>
#include <sstream>
#include <system_error>

class ThreadBase {
private:
  std::promise<void> pObj;
  std::future<void>  fObj;
  std::thread tObj;
protected:
  virtual void Action(void) = 0;
  bool Running(void) {
    static const auto zero_ms = std::chrono::milliseconds(0);
    return fObj.wait_for(zero_ms) == std::future_status::timeout;
    }
public:
  ThreadBase() : fObj(pObj.get_future()) {}

  ThreadBase(ThreadBase&& other) : pObj(std::move(other.pObj)), fObj(std::move(other.fObj))
    { std::swap(tObj,other.tObj); }

  virtual ~ThreadBase() {
    Cancel();
    Join();
    }

  ThreadBase& operator=(ThreadBase&& other)  {
    pObj = std::move(other.pObj);
    fObj = std::move(other.fObj);
    std::swap(tObj,other.tObj);
    return *this;
    }

  bool Start(void) {
    try {
       if (tObj.joinable() and not Running()) {
          // task finished - reinitialize.
          Join();
          std::promise<void> p;
          std::future<void> f(p.get_future());
          pObj = std::move(p);
          fObj = std::move(f);
          }
       if (not tObj.joinable()) {
          std::thread t([&]() { Action(); });
          std::swap(t, tObj);
          }
       }
    catch(const std::system_error& e) { return false; }
    return true;
    }

  void Cancel(void) {
    if (Running())
       pObj.set_value();
    }

  void Join(void) {
    if (tObj.joinable())
       tObj.join();
    }

  size_t ThreadId(void) {
    std::stringstream ss;
    ss << tObj.get_id();
    return std::stoll(ss.str());
    }

  void SleepMinutes(size_t Minutes) {
    for(size_t i=0; Running() && i<Minutes; i++)
       SleepSeconds(60);
    }

  void SleepSeconds(size_t Seconds) {
    for(ssize_t ms = Seconds * 1000; Running() && ms > 0; ms -= 100)
       std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};
