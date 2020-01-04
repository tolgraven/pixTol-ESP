#pragma once

#include <map>
#include <Arduino.h>
#include "log.h"
#include "watchdog.h"
#include "buffer.h"

#ifdef ESP8266
#define STACK_END 0x3fffeb30
#else
#define STACK_END 0x00000000
#endif

class Debug {
  uint32_t startTime = 0, lastFlush = 0;
  uint16_t flushEverySeconds = 5;
  uint32_t dmxFrameCounter = 0;
  uint8_t* stackStart; //for a generalized approach...

  std::map<Buffer*, uint16_t> buffers;

  public:
  Debug(uint8_t* stackStartDummy):
    stackStart(stackStartDummy) {}

  bool sendIfChanged(const String& property, int value);
  void logFunctionChannels(uint8_t* dataStart, const String& id, uint8_t expectedHz = 40, uint8_t num = 12);
  void registerToLogEvery(Buffer& buffer, uint16_t seconds);
  void logDMXStatus(uint8_t* data, const String& id = "SRC"); // for logging and stuff... makes sense?
  void logAndSave(const String& msg); //possible approach...  log to serial + save message, then post by LN once MQTT up

  void run();

  void stackAvailableLog() {
    uint8_t stackNow = 0;
    lg.f("Stack", Log::INFO, "From: %x, start: %x, now: %p, used: %d\n",
        STACK_END, 0x3fffffb0, &stackNow, stackUsed());
  }
  uint16_t stackUsed() {
    uint8_t stackNow = 0;
    uint16_t stackUsed = (uint8_t*)0x3fffffb0 - &stackNow;
    return stackUsed;
  }

  enum BootStage: uint8_t { doneBOOT = 0, doneHOMIE, doneMAIN, doneONLINE };
  static int ms[doneONLINE + 1];
  static int heap[doneONLINE + 1];

  static void bootLog(BootStage bs);
  static void bootInfoPerMqtt();
}; // possible to get entire stacktrace in memory? if so mqtt -> decoder -> auto proper trace?
   // dont need if just keep connected to Pi, but...
