#pragma once

#include <functional>
#include <Arduino.h>
#include <Ticker.h> //should have functional stuff nowadays!!
#include "log.h"

#if defined(ARDUINO_ARCH_ESP8266) // so is it ESP8266 / ESP32 or ARDUINO_ARCH_*? or both?
extern "C" {
#include "user_interface.h"
}
#define __ESP32_BAIL(ret) {}

#elif defined(ARDUINO_ARCH_ESP32 ) // so is it ESP8266 / ESP32 or ARDUINO_ARCH_*? or both?
 #ifndef ICACHE_RAM_ATTR // ESP32 doesn't define ICACHE_RAM_ATTR
  #define ICACHE_RAM_ATTR IRAM_ATTR
 #endif
extern "C" {
#include <esp_system.h>
}
#define __ESP32_BAIL(ret) { return ret; }
#endif

namespace tol {

// XXX move these to sep file obviously...
// and come up with good numbering scheme
// #define PIXTOL_BOOT_BLABLA            31
#define PIXTOL_INPUT_ARTNET              32
#define PIXTOL_INPUT_SACN                33
#define PIXTOL_INPUT_DMX                 34
#define PIXTOL_INPUT_BLA                 35

#define PIXTOL_SCHEDULER_ENTRY           41
#define PIXTOL_SCHEDULER_INPUT_DMX       42
// #define PIXTOL_RENDERER_ENTRY
#define PIXTOL_RENDERER_UPDATE_TARGET    50
#define PIXTOL_RENDERER_FRAME            51
#define PIXTOL_UPDATER                   60
#define PIXTOL_UPDATER_LOOP              61
// submodules for updaters or? but figure out how run that sorta
// shit without nuking mem or having five billion defines
// since can't fit shit in RTC
// best just keep lookup table on server -> done
// module baselines and then hash off individual ID?
#define PIXTOL_OUTPUT_STRIP             110
#define PIXTOL_OUTPUT_ARTNET            112
// put these in own and use enum stuff so its not fucked.



/* pixtol loop watchdog, based on mdEspRestart
  Each loop iteration, first feed wdt:      feed();
  Stamp module ID before each part of loop: stamp(MODULE_ID);
  At end of loop, stamp wdt as finished:    stamp();
  User defined ids are 32 bit values from 1 to 4,294,967,294 = 0xffffffff-1.
  Can identify modules, exception causes, or user defined IDs.
  ^^XXX what's exc range? avoid (tho not actually needed)
  Max uint32_t value, 0xffffffff = LWD_ID_LOOP_END marks end of loop() */
#define LWD_ID_INVALID         0x0
#define LWD_ID_LOOP_END        0xFFFFFFFF
// #define LWD_DEFAULT_TIMEOUT     12000  // 12 seconds, or twice hw wd timeout
// #define LWD_DEFAULT_TIMEOUT      5000  // 5 seconds, just under reg wd
#define RTC_SYSTEM                 64  // RTC: system occupies 64, user other 128...
#define RTC_USER                  128
#define LWD_RTC_INVALID_OFFSET   0xFF

/* We extend original ESP.getResetInfoPtr()->reason table of: */
#ifndef ESP8266
#define     REASON_DEFAULT_RST       0 // normal startup by power on
#define     REASON_WDT_RST           1 // hardware watchdog reset
#define     REASON_EXCEPTION_RST     2 // exception reset
#define     REASON_SOFT_WDT_RST      3 // software watchdog reset
#define     REASON_SOFT_RESTART      4 // software restart, system_restart
#define     REASON_DEEP_SLEEP_AWAKE  5 // wake up from deep-sleep
#define     REASON_EXT_SYS_RST       6 // external system reset
#endif
#define LWD_REASON_USER_RESET        7 // user initiated reset, see userReset()
#define LWD_REASON_USER_RESTART      8 // user initiated restart, see userRestart()
#define LWD_REASON_LOOP_RST          9 // lwdt hit within loop()
#define LWD_REASON_BETWEEN_RST      10 // lwdt hit between end of loop() and next
#define LWD_REASON_CORRUPT          11 // lwd RTC storage overwritten
#define LWD_REASON_SETUP            12 // lwdt hit during setup() - XXX implement
#define LWD_REASON_INVALID          13 // for bounds checking
// used when checking for memory corruption
#define LWD_INVALID_FLAG      0x00 // 0000 0000
#define LWD_RESTART_FLAG      0xA5 // 1010 0101
#define LWD_HANDLED_FLAG      0xA1 // 1010 0001
#define LWD_USER_FLAG_MASK    0xFB // 1111 1011

/* Restart data. Two attributes(1) anally assure RTC compatibility:
  (packed) unneeded as-is, but if members uneven would pad, aligning last member on 4 byte boundary.
  (aligned(4)) puts instance on 4 byte boundary, for system_rtc_mem_read / system_rtc_write(2).
  Also unneeded here, as struct contains a 32 bit member and auto aligns.

  (1). GCC doc on attributes: Specifying Attributes of Variables
  https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Variable-Attributes.html#Variable%20Attributes
  (2). ESP8266 SDK API Reference, Version 2.2.1 (2018/05) "3.3.23 system_rtc_mem_write" p. 19
  https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf#page=1&zoom=auto,-13,792 */
using lwdFlag_t   = uint8_t;
using lwdReason_t = uint8_t;
using lwdCount_t  = uint16_t;
using lwdID_t     = uint32_t; //32bit being size of exccause
struct __attribute__((packed)) {
// struct __attribute__((packed)) LwdData {
  lwdFlag_t     flag; // 1 byte structure flag
  lwdReason_t reason; // 1 byte reason
  lwdCount_t   count; // 2 byte (16 bit) count of consecutive boots for same reason
  lwdID_t         id; // 4 byte (32 bit) id (module id, user defined or exception cause)
} wd __attribute__((aligned(4)));
// } __attribute__((aligned(4)));
// LwdData wd;

// void rtcWrite() {
// #ifdef ARDUINO_ARCH_ESP8266 // so is it ESP8266 / ESP32 or ARDUINO_ARCH_*? or both?
//     system_rtc_mem_write(rtcAddress, &wd, sizeof(wd));
// #endif
// #ifdef ARDUINO_ARCH_ESP32
// #endif
//     }

class LoopWatchdog {
  Ticker wdt;
  uint32_t timeAliveBeforeSafe = 300000; // should actually prob be very long since same bug can still bite late but repeatedly. tweak later...
  static const uint32_t defaultTimeout = 24000;
  volatile uint32_t fedAt, lapseAt, timeout, negTimeout; //so neg is not actually neg since uint, just gets negated twice bc uh
  bool doneGet = false; // Flag to avoid incrementing count more than once.
  static constexpr uint8_t rtcBlockSize = (sizeof(wd) + 3) & (~3);
  static constexpr uint8_t rtcMaxValidAddress = RTC_SYSTEM + RTC_USER - (rtcBlockSize / 4); //starts at max
  uint8_t rtcAddress = rtcMaxValidAddress;
  bool enabled = true;


  bool validFlag() { return (wd.flag & LWD_USER_FLAG_MASK) == LWD_HANDLED_FLAG; } // welp update and ensure still makes sense when expanding former USER_FLAG
  void save() {
#ifdef ESP8266
    system_rtc_mem_write(rtcAddress, &wd, sizeof(wd));
#endif
  }
  void load() {
    __ESP32_BAIL()
#ifdef ESP8266
    system_rtc_mem_read(rtcAddress, &wd, sizeof(wd));
#endif
    if(!validFlag() || wd.reason >= LWD_REASON_INVALID) { // appeears nothing (valid) yet in RTC, try init
      wd.flag = LWD_RESTART_FLAG; // non-LWD default should take value of 0x00 tho btw...
      wd.reason = REASON_DEFAULT_RST;
      wd.count = 0;
      wd.id = LWD_ID_LOOP_END;
    }
  }

  public:
  LoopWatchdog(uint32_t timeout = defaultTimeout, uint8_t rtcBlock = 0):
    timeout(timeout), negTimeout(-timeout) { // constructor called once when creating loop wdt, optionally setting custom timeout interval
      setRTCAddress(rtcBlock);
      lg.dbg("Done Watchdog"); // should be safe now even if pre-init right
  }

  void init() { // actually start watchdog...
    __ESP32_BAIL()

    setEnabled(true);
    auto cb = std::bind(&LoopWatchdog::callback, this);
    wdt.attach_ms(timeout, cb); //lower than reg or wont save where we are...
    stamp(LWD_ID_LOOP_END); //= start of loop()
    feed();
  }
  void stop() { wdt.detach(); setEnabled(false); }
  void setEnabled(bool state = true) { enabled = state; }

  void feed() { // Called at start of loop: feed and reset loop wdt.
    __ESP32_BAIL()
    fedAt = millis();
    if(fedAt > timeAliveBeforeSafe && wd.count != 0) { // makes sense as configurable val tho. uptime before "success"
      // bc if long enough always posible to sneak in an ota flash right
      DEBUG("Up long enough. Resetting dirty restart count.");
      wd.count = 0;
      save(); // haha this usually hits in the middle of OTA update and crashes it.
      // so we need to have some event/flag for whether is safe.
    }
    lapseAt = fedAt + timeout;
    if(wd.id != LWD_ID_LOOP_END) {
      setReason(LWD_REASON_BETWEEN_RST);
      // lg.dbg("wd.id changed between loops: " + wd.id);
      // ESP.restart(); //ehh better just really serious logging? or? still broken too - happens reg boot
    }
  }
  void stamp(lwdID_t id = LWD_ID_LOOP_END) {
    wd.id = id;
  } //checkin each module start, closes loop if no ID passed


  void ICACHE_RAM_ATTR callback() { // void callback() { //i guess ICACHE_... if by interrupt, else non?
    if(!enabled || millis() - fedAt < timeout) {
      return; // all good, keep going
    } else if(timeout != lapseAt - fedAt // are these actually any measure of useful? very unstable hw good to sep tho.
           || timeout != -negTimeout
           || !validFlag()) {
      setReason(LWD_REASON_CORRUPT, LWD_HANDLED_FLAG); // something's been overwritten
    } else {
      setReason(LWD_REASON_LOOP_RST, LWD_HANDLED_FLAG); // hit watchdog timer...
    }
#ifdef ESP8266
    ESP.restart(); // timeout or memory correuption - restart. But want to dump things etc so can investigate not just blow it up tho.
#endif
  }

  uint8_t getSystemReason() {
#ifdef ESP8266
    return ESP.getResetInfoPtr()->reason;
#endif
    return 0;
  }
  uint32_t getSystemExceptionCause() {
#ifdef ESP8266
    return ESP.getResetInfoPtr()->exccause;
#endif
    return 0;
  }


  void setReason(lwdReason_t reason, lwdFlag_t flag = 0) { //sets reason, incrementing count if same, and optionally modifying flag
  // void setReason(lwdReason_t reason, lwdFlag_t flag = LWD_HANDLED_FLAG) { //sets reason, incrementing count if same, and optionally modifying flag
    bool repeatCause = (reason == wd.reason);  // same ast last
    if(reason == REASON_EXCEPTION_RST) { //just override here yeh..
      lwdID_t exceptionCause = getSystemExceptionCause();
      repeatCause = repeatCause && (exceptionCause == wd.id); //not same if different ids = different exceptions
      wd.id = exceptionCause;
    }
    if(repeatCause) wd.count++;
    else wd.count = 1;

    wd.reason = reason;
    if(flag) wd.flag = flag;
    save(); // will also store wd.id when already set by stamp
  }

  // lwdReason_t getReason(lwdCount_t& count, lwdID_t& id) { // change to just return a struct instead?
  lwdReason_t getReason() {
    if(!doneGet) {
      load();
      lwdReason_t reason = getSystemReason();
      // if(wd.flag == LWD_HANDLED_FLAG && reason == REASON_SOFT_RESTART)
      if(wd.flag == LWD_HANDLED_FLAG) // use flag for everything we do. what to do tho is always set _HANDLED then any extra on top. ie re-introduce _USER etc :P
        reason = wd.reason; // keep what we set, not system value.
      // setReason(reason, LWD_RESTART_FLAG); // store "winning" value
      setReason(reason); // store "winning" value
      doneGet = true; //once per boot since we do the incrementing
    }
    // id = wd.id;
    // count = wd.count;
    return wd.reason;
  }

  void userRestart(lwdID_t id = 0, lwdReason_t reason = LWD_REASON_USER_RESTART) {// Does ESP.restart() (default) or ESP.reset(), setting id if > 0.  Count will ignore id, only considering reason
    if(id) wd.id = id; //wd.id = id? id: wd.id; // only put id if actively set
    setReason(reason, LWD_HANDLED_FLAG);
#ifdef ESP8266
    if(reason == LWD_REASON_USER_RESTART) {
      ESP.restart();
    } else if(reason == LWD_REASON_USER_RESET) {
      ESP.reset();
    }
#endif
  }

  std::string getResetReason() { // Returns human-readable reset info, like ESP.getResetReason(), but also reporting our five additional reasons.
    return "not implemented";
    // lwdCount_t count;
    // lwdID_t id;
    // lwdReason_t restartReason = getReason(count, id);
    // std::string countStr = ", count: " + (std::string)count + ", last stamp " + wd.id;
    // std::string idStr = " " + (std::string)id + countStr;
    // lwdReason_t restartReason = getReason();
    // std::string countStr = ", count: " + std::to_string(wd.count) + ", last stamp/id " + wd.id;
    // std::string idStr = " id " + (std::string)wd.id + countStr;
// #if defined(ESP8266)
    // auto mainReason = ESP.getResetReason();
// #elif defined(ESP32)
    // std::string mainReason = "Not impl";
// #endif
    // return ("Reset:  " + (
    //  (restartReason == REASON_EXCEPTION_RST)?    mainReason + idStr:
    //  (restartReason <= REASON_EXT_SYS_RST)?      mainReason + countStr:
    //  (restartReason == LWD_REASON_USER_RESET)?   "User reset, id" + idStr:
    //  (restartReason == LWD_REASON_USER_RESTART)? "User restart, id" + idStr:
    //  (restartReason == LWD_REASON_LOOP_RST)?     "Loop watchdog in" + idStr:
    //  (restartReason == LWD_REASON_BETWEEN_RST)?  "Loop watchdog incomplete in" + idStr:
    //  (restartReason == LWD_REASON_CORRUPT)?      "Loop watchdog overwritten in" + idStr:
    //  mainReason));
  }
  int getCount() { return (int)wd.count; }

  // Called before start to change RTC storage address. User memory has 128 4-byte blocks,
  // we use 2, so valid range 0 to 126 (default) Returns false if block invalid. OR XXX block busy by other storage we know of?
  bool setRTCAddress(uint8_t addr = LWD_RTC_INVALID_OFFSET) {
    if((uint16_t)addr + RTC_SYSTEM <= rtcMaxValidAddress) {
      rtcAddress = addr + RTC_SYSTEM; // gotta offset.
      return true;
    } else return false;
  }

  bool softResettable() {; // false if fw just flashed by UART and ESP can't be soft reset (hw bug) but needs RST pin or power cycle
    int bootmode = 0;
    asm ("movi %0, 0x60000200\n\t"
         "l32i %0, %0, 0x118\n\t"
         : "+r" (bootmode) /* Output */
         : /* Inputs (none) */
         : "memory" /* Clobbered */ );
    return (((bootmode >> 0x10) & 0x7) != 1);
  }
};

extern LoopWatchdog lwd;

}
