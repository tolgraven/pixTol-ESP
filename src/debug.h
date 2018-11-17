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

// #ifndef UNIT_TESTING //something like that, redefine logger to simple cout for native testing
// class Log: public HomieNode {
// public:
//   Log(): level(DEBUG), serial(true), mqtt(true), HomieNode("Log", "Logger") {
//     for(auto s; {"Level", "Serial", "MQTT"}) advertise(s).settable();
//   }
// 	enum Level: int8_t { INVALID=-1, DEBUG=0, INFO, WARNING, ERROR, CRITICAL }; //add an "NONE" at end to disable output fully?
//   static const Level defaultLevel = DEBUG;
//
// 	virtual bool handleInput(const String& property, const HomieRange& range, const String& value) override {
//     LN.logf("Log settings", DEBUG, "requested setting property %s to %s", property.c_str(), value.c_str());
//     if(property == "Level") {
//       Level newLevel = convert(value);
//       if(newLevel == INVALID) {
//         logf("Log settings", WARNING, "Received invalid level %s.", value.c_str());
//         return false;
//       }
//       _level = newLevel;
//       logf("Log settings", INFO, "Level now %s", value.c_str());
//       setProperty("Level").send(convert[_level]);
//       return true;
//     } else if(property == "Serial") {
//       bool on = (value == "ON");
//       serial = on;
//       LN.logf("Log settings", INFO, "Serial output %s", on? "on": "off");
//       setProperty("Serial").send(on? "On": "Off");
//       return true;
//     }
//     logf("Log settings", ERROR, "Invalid property %s, value %s", property.c_str(), value.c_str());
//     return false;
//   }
//
// 	void log(const String location, const Level level, const String text) const {
//     if (!shouldLog(level)) return;
//     String mqtt_path(convert[level]);
//     mqtt_path.concat('/');
//     mqtt_path.concat(function);
//     if (mqtt   && Homie.isConnected())  setProperty(mqtt_path).setRetained(false).send(text);
//     if (serial || !Homie.isConnected()) Serial.printf("%d: %s:%s\n", millis(), mqtt_path.c_str(), text.c_str());
//   }
// 	void logf(const String location, const Level level, const char *format, ...) const {
//     if (!shouldLog(level)) return;
//     va_list arg;
//     va_start(arg, format);
//     char temp[100]; // char* buffer = temp;
//     size_t len = vsnprintf(temp, sizeof(temp), format, arg);
//     va_end(arg);
//     log(function, level, temp);
//   }
//
// 	bool shouldLog(Level lvl) const { return ((uint_fast8_t) lvl >= (uint_fast8_t) _level); }
// 	void setLevel(Level lvl) { if(lvl >= DEBUG && lvl <= CRITICAL) _level = l; }
//
// private:
// 	Level _level;
// 	bool serial;
//   bool mqtt;
//
// 	static Level convert(const String& lvl) {
//     String lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
//     for(uint8_t iLevel = DEBUG; iLevel <= CRITICAL; iLevel++)
//       if(_level.equalsIgnoreCase(lvlStr[iLevel])) return static_cast<Level>(iLevel);
//     return INVALID;
//   }
// 	static String convert(const Level lvl) {
//     String lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
//     auto index = constrain(lvl, INVALID, CRITICAL+1);
//     if(index >= 0) return lvlStr[index];
//     else return INVALID;
//   }
// };
//
// Log log;
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

