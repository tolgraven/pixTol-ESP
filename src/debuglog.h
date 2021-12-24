#pragma once

#include <map>
#include <Arduino.h>
#ifdef ESP8266
#include <EspSaveCrash.h>
#endif

#include "log.h"
#include "watchdog.h"
#include "buffer.h"

#ifdef ESP8266
#define STACK_END 0x3fffeb30
#define STACK_TOP 0x3fffffb0
#else
#define STACK_END 0x00000000
#define STACK_TOP 0x00000000
#endif

namespace tol {

class Debug {
  uint32_t startTime = 0, lastFlush = 0;
  uint16_t flushEverySeconds = 5;
  uint32_t dmxFrameCounter = 0;
  uint8_t* stackAtStart; //for a generalized approach...

  std::map<Buffer*, uint16_t> buffers;


  public:
  Debug(uint8_t* stackAtStartDummy):
    stackAtStart(stackAtStartDummy) {}

  bool sendIfChanged(const std::string& property, int value);
  void logFunctionChannels(uint8_t* dataStart, const std::string& id, uint8_t expectedHz = 40, uint8_t num = 12);
  void registerToLogEvery(Buffer& buffer, uint16_t seconds);
  void logDMXStatus(uint8_t* data, const std::string& id = "SRC"); // for logging and stuff... makes sense?
  void logAndSave(const std::string& msg); //possible approach...  log to serial + save message, then post by LN once MQTT up

  void run();

  void stackAvailableLog() {
    uint8_t stackNow = 0;
    lg.f("Stack", tol::Log::INFO, "At boot: %x, capacity: %x, now: %p, used: %d or %d\n",
        STACK_END, STACK_TOP, &stackNow, stackUsed(), stackAtStart - &stackNow);
  }
  uint16_t stackUsed() {
    uint8_t stackNow = 0;
    uint16_t stackUsed = (uint8_t*)STACK_TOP - &stackNow;
    return stackUsed;
  }

  // EspSaveCrash saveCrash((uint16_t)0x0010, (uint16_t)0x1000); // original 0x0200 / 512 barely fits fucking one... dumb...
  // EspSaveCrash saveCrash{(uint16_t)0x0010, (uint16_t)0x1000}; ; // original 0x0200 / 512 barely fits fucking one... dumb...
  // std::string getStacktrace() {
  //   // unique_ptr<char[]> lastStacktraceBuffer = new char[2048];
  //   // std::unique_ptr<char*> lastStacktraceBuffer(new char[2048]);
  //   // std::unique_ptr<char[]> lastStacktraceBuffer = new char[2048];
  //   // std::unique_ptr<char[]> lastStacktraceBuffer(new char[2048]); // Allocate a buffer to store contents of the file.
  //   char* lastStacktraceBuffer = new char[2048];
  //   saveCrash.crashToBuffer(lastStacktraceBuffer);
  //   std::string outStr(lastStacktraceBuffer);
  //   delete[] lastStacktraceBuffer;
  //   // return std::string(*lastStacktraceBuffer);
  //   return outStr; //wont work right right?
  // }

  enum BootStage: uint8_t { doneBOOT = 0, doneHOMIE, doneMAIN, doneONLINE };
  static int ms[doneONLINE + 1];
  static int heap[doneONLINE + 1];

  static void bootLog(BootStage bs);
  static void bootInfoPerMqtt();
}; // possible to get entire stacktrace in memory? if so mqtt -> decoder -> auto proper trace?
   // dont need if just keep connected to Pi, but...
}
