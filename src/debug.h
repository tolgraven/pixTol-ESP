#pragma once

#include <map>
#include <LoggerNode.h>
#include "config.h"
#include "state.h"
#include "iot.h"

#define __pfunc__ __PRETTY_FUNCTION__

#define _DEBUG    LoggerNode::DEBUG
#define _INFO     LoggerNode::INFO
#define _WARNING  LoggerNode::WARNING
#define _ERROR    LoggerNode::ERROR
#define _CRITICAL LoggerNode::CRITICAL

#define _DEBUG_ __func__, LoggerNode::DEBUG

// If you use std libs like std::vector, make sure to call its ::reserve() method
// before filling it. This allows allocating only once, which reduces mem
// fragmentation, and makes sure that there are no empty unused slots left over in
// the container at the end.

bool sendIfChanged(HomieNode& hn, const String& property, int value) {
  static std::map<String, int> propertyValues;
  if(propertyValues.find(property) != propertyValues.end()) {
    int last = propertyValues[property];
    if(value == last || (value > 0.90*last && value < 1.10*last)) //XXX settable tolerance...
      return false;
  }
  hn.setProperty(property).send(String(value)); //new property or new value...
  propertyValues[property] = value;
  return true; //set and sent
}

void logDMXStatus(uint16_t universe, uint8_t* data, uint16_t length) { // for logging and stuff... makes sense?
  static int first = millis();
  static uint16_t dmxFrameCounter = 0;
  dmxFrameCounter++;

  if(!(dmxFrameCounter % (cfg->dmxHz.get()*10))) { // every 10s (if input correct and stable)
    uint16_t totalTime = millis() - first;
    first = millis();
    sendIfChanged(statusNode, "freeHeap", ESP.getFreeHeap());
    sendIfChanged(statusNode, "heapFragmentation", ESP.getHeapFragmentation());
    sendIfChanged(statusNode, "maxFreeBlockSize", ESP.getMaxFreeBlockSize());
    sendIfChanged(statusNode, "fps", dmxFrameCounter / (totalTime / 1000));
    sendIfChanged(statusNode, "droppedFrames", s->droppedFrames);

    sendIfChanged(statusNode, "dimmer.base", f->dimmer.getByte());
    sendIfChanged(statusNode, "dimmer.force", f->chOverride[chDimmer]); //whyyy this gets spammed when no change?
    sendIfChanged(statusNode, "dimmer.out", f->outBrightness);
    dmxFrameCounter = 0;
  }
  switch(lastArtnetOpCode) { // check opcode for logging purposes, actual logic in callback function
    case OpDmx:
      break;
    case OpPoll:
      if(cfg->logArtnet.get() >= 2) LN.logf("artnet", LoggerNode::DEBUG, "Art Poll Packet");
      break;
  }
}

void logAndSave(const String& msg) { //possible approach...  log to serial + save message, then post by LN once MQTT up
  // cause now lose info if never connects...
}

/* #define LOGF() */
/* enum Log { */
/*   INVALID = -1, DEBUG = 0, INFO, WARNING, ERROR, CRITICAL */
/* }; */

// in case do more hardcore watchdog stuff...
// struct bootflags {
//   unsigned char raw_rst_cause: 4;
//   unsigned char raw_bootdevice: 4;
//   unsigned char raw_bootmode: 4;
//
//   unsigned char rst_normal_boot: 1;
//   unsigned char rst_reset_pin: 1;
//   unsigned char rst_watchdog: 1;
//
//   unsigned char bootdevice_ram: 1;
//   unsigned char bootdevice_flash: 1;
// };
//
// struct bootflags bootmode_detect(void) {
//   int reset_reason, bootmode;
//   asm ("movi %0, 0x60000600\n\t"
//        "movi %1, 0x60000200\n\t"
//        "l32i %0, %0, 0x114\n\t"
//        "l32i %1, %1, 0x118\n\t"
//        : "+r" (reset_reason), "+r" (bootmode) /* Outputs */
//        : /* Inputs (none) */
//        : "memory" /* Clobbered */);
//
//   struct bootflags flags;
//
//   flags.raw_rst_cause = (reset_reason & 0xF);
//   flags.raw_bootdevice = ((bootmode >> 0x10) & 0x7);
//   flags.raw_bootmode = ((bootmode >> 0x1D) & 0x7);
//
//   flags.rst_normal_boot = flags.raw_rst_cause == 0x1;
//   flags.rst_reset_pin = flags.raw_rst_cause == 0x2;
//   flags.rst_watchdog = flags.raw_rst_cause == 0x4;
//
//   flags.bootdevice_ram = flags.raw_bootdevice == 0x1;
//   flags.bootdevice_flash = flags.raw_bootdevice == 0x3;
//
//   return flags;
// }

class Debug {
  public:
  HomieNode* status;
  Strip* s;
  Functions* f;
  ConfigNode* cfg;

  Debug(HomieNode* status, Strip* s, Functions* f, ConfigNode* cfg):
    status(status), s(s), f(f), cfg(cfg) {
  }

  bool sendIfChanged(const String& property, int value) {
    static std::map<String, int> propertyValues;
    if(propertyValues.find(property) != propertyValues.end()) {
      int last = propertyValues[property];
      if(value == last || (value > 0.90*last && value < 1.10*last)) //XXX settable tolerance...
        return false;
    }
    status->setProperty(property).send(String(value)); //new property or new value...
    propertyValues[property] = value;
    return true; //set and sent
  }

  void logDMXStatus(uint16_t universe, uint8_t* data) { // for logging and stuff... makes sense?
    static int first = millis();
    static uint16_t dmxFrameCounter = 0;
    dmxFrameCounter++;

    if(!(dmxFrameCounter % (cfg->dmxHz.get()*10))) { // every 10s (if input correct and stable)
      uint16_t totalTime = millis() - first;
      first = millis();
      // static const String keys[] = { "freeHeap", "heapFragmentation", "maxFreeBlockSize", "fps", "droppedFrames",
      //   "dimmer.base", "dimmer.force", "dimmer.out"};
      // static const std::function<int()> getters[] =
      // { ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize(),
      //   [=]() {dmxFrameCounter / (totalTime / 1000)},
      //   [s]() {return s->droppedFrames}, [f]() {return f->dimmer.getByte()}, [f]() {return f->chOverride[chDimmer]},
      // [f]() {return f->outBrightness}};
      // static map<String, string> table(make_map(keys, getters));

      sendIfChanged("freeHeap", ESP.getFreeHeap());
      sendIfChanged("heapFragmentation", ESP.getHeapFragmentation());
      sendIfChanged("maxFreeBlockSize", ESP.getMaxFreeBlockSize());
      sendIfChanged("fps", dmxFrameCounter / (totalTime / 1000));
      sendIfChanged("droppedFrames", s->droppedFrames);

      sendIfChanged("dimmer.base", f->chan[chDimmer]->getByte());
      sendIfChanged("dimmer.force", f->chOverride[chDimmer]); //whyyy this gets spammed when no change?
      sendIfChanged("dimmer.out", f->outBrightness);
      dmxFrameCounter = 0;
    }
  }

  void logAndSave(const String& msg) { //possible approach...  log to serial + save message, then post by LN once MQTT up
    // cause now lose info if never connects...
  }

  static int getBootDevice(void) {
    int bootmode;
    asm ("movi %0, 0x60000200\n\t"
        "l32i %0, %0, 0x118\n\t"
        : "+r" (bootmode) /* Output */
        : /* Inputs (none) */
        : "memory" /* Clobbered */);
    return ((bootmode >> 0x10) & 0x7);
  }
  static void resetInfoLog() {
    switch (ESP.getResetInfoPtr()->reason) {
      // USUALLY OK ONES:
      case REASON_DEFAULT_RST:      LN.logf("Reset", LoggerNode::DEBUG, "Power on"); break; // normal power on
      case REASON_SOFT_RESTART:     LN.logf("Reset", LoggerNode::DEBUG, "Software/System restart"); break;
      case REASON_DEEP_SLEEP_AWAKE: LN.logf("Reset", LoggerNode::DEBUG, "Deep-Sleep Wake"); break;
      case REASON_EXT_SYS_RST:      LN.logf("Reset", LoggerNode::DEBUG, "External System (Reset Pin)"); break;
      // NOT GOOD AND VERY BAD:
      case REASON_WDT_RST:          LN.logf("Reset", LoggerNode::DEBUG, "Hardware Watchdog"); break;
      case REASON_SOFT_WDT_RST:     LN.logf("Reset", LoggerNode::DEBUG, "Software Watchdog"); break;
      case REASON_EXCEPTION_RST:    LN.logf("Reset", LoggerNode::DEBUG, "Exception"); break;
      default:                      LN.logf("Reset", LoggerNode::DEBUG, "Unknown"); break;
    }
  }

  enum BootStage: uint8_t { doneBOOT = 0, doneHOMIE, doneMAIN, doneONLINE };
  static int ms[doneONLINE + 1];
  static int heap[doneONLINE + 1];

  static void bootLog(BootStage bs) {
    Debug::ms[bs] = millis();
    Debug::heap[bs] = ESP.getFreeHeap();
    switch(bs) { // eh no need unless more stuff comes
      case doneBOOT: break;
      case doneHOMIE: break;
      case doneMAIN: break;
    }
  }

  // possible to get entire stacktrace in memory? if so mqtt -> decoder -> auto proper trace?
  // or keep dev one connected to another esp forwarding...
  static void bootInfoPerMqtt() {
    if(getBootDevice() == 1) LN.logf("SERIAL-FLASH-BUG", LoggerNode::WARNING, "%s\n%s", "First boot post serial flash. OTA won't work and first WDT will brick device until manually reset", "Best do that now, rather than later");
    resetInfoLog();
    LN.logf("Reset", LoggerNode::DEBUG, "%s", ESP.getResetInfo().c_str());
    LN.logf("Boot", LoggerNode::DEBUG, "Free heap post boot/homie/main/wifi: %d / %d / %d / %d",
                      heap[doneBOOT], heap[doneHOMIE], heap[doneMAIN], heap[doneONLINE]);
    LN.logf("Boot", LoggerNode::DEBUG, "ms for homie/main, elapsed til setup()/wifi: %d / %d,  %d / %d",
        Debug::ms[doneHOMIE] - Debug::ms[doneBOOT], Debug::ms[doneMAIN] - Debug::ms[doneHOMIE], Debug::ms[doneMAIN] - Debug::ms[doneBOOT], Debug::ms[doneONLINE] - Debug::ms[doneBOOT]);
  }
};

