#pragma once

#include <map>
// #include <execinfo.h> // would be cool for stacktraces but not available :<
#include <Homie.h>
#include "config.h"
#include "io/strip.h"
#include "functions.h"


class Debug {
  public:
  HomieNode* status;
  Strip* s;
  Functions* f;
  ConfigNode* cfg;
  int first = 0;
  uint16_t dmxFrameCounter = 0;

  Debug(HomieNode* status, Strip* s, Functions* f, ConfigNode* cfg):
    status(status), s(s), f(f), cfg(cfg) {}

  bool sendIfChanged(const String& property, int value);
  void logFunctionChannels(uint8_t* dataStart, const String& id, uint8_t num = 12);
  void logDMXStatus(uint8_t* data); // for logging and stuff... makes sense?
  void logAndSave(const String& msg); //possible approach...  log to serial + save message, then post by LN once MQTT up

  enum BootStage: uint8_t { doneBOOT = 0, doneHOMIE, doneMAIN, doneONLINE };
  static int ms[doneONLINE + 1];
  static int heap[doneONLINE + 1];

  static int getBootDevice(void);
  static void resetInfoLog();
  static void bootLog(BootStage bs);
  static void bootInfoPerMqtt();
}; // possible to get entire stacktrace in memory? if so mqtt -> decoder -> auto proper trace?
  // or keep dev one connected to another esp forwarding...
