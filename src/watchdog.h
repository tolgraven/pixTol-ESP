#pragma once

#include <functional>
#include "Arduino.h"
#include <Ticker.h>
extern "C" {
#include "user_interface.h"
}
/* pixtol loop watchdog, based on mdEspRestart
  Each loop iteration, first feed wdt:      lwdtFeed();
  Stamp module ID before each part of loop: lwdtStamp(MODULE_ID);
  At end of loop, stamp wdt as finished:    lwdtStamp();
  User defined ids are 32 bit values from 1 to 4,294,967,294 = 0xffffffff-1.
  Can identify modules, exception causes, or user defined IDs.
  ^^XXX what's exc range? avoid (tho not actually needed)
  Max uint32_t value, 0xffffffff = LWD_ID_LOOP_END marks end of loop() */
// using lwdFlag_t   = uint8_t;
// using lwdReason_t = uint8_t;
using lwdFlag_t, lwdReason_t = uint8_t;
using lwdCount_t             = uint16_t; //should be uint8_t? when'd you ever up cap > 255?
using lwdID_t                = uint32_t; //32 due to size of exccause.... cast?
#define LWD_ID_INVALID        0x0
#define LWD_ID_LOOP_END 0xFFFFFFFF
#define LWD_TIMEOUT          12000  // 12 seconds, or twice hw wd timeout
#define LWD_RTC_ADDRESS       0xFF  // this (default) address is invalid, set correct when we see it

/* We extend original ESP.getResetInfoPtr()->reason table of:
        REASON_DEFAULT_RST       0    normal startup by power on
        REASON_WDT_RST           1    hardware watch dog reset
        REASON_EXCEPTION_RST     2    exception reset
        REASON_SOFT_WDT_RST      3    software watch dog reset
        REASON_SOFT_RESTART      4    software restart ,system_restart
        REASON_DEEP_SLEEP_AWAKE  5    wake up from deep-sleep
        REASON_EXT_SYS_RST       6    external system reset */
#define LWD_REASON_USER_RESET    7 // user initiated reset, see userRestart()
#define LWD_REASON_USER_RESTART  8 // user initiated restart, see userRestart()
#define LWD_REASON_LOOP_RST      9 // lwdt hit within loop()
#define LWD_REASON_BETWEEN_RST  10 // lwdt hit between end of loop() and next
#define LWD_REASON_CORRUPT      11 // lwd RTC storage overwritten
#define LWD_REASON_SETUP        12 // lwdt hit during setup() - XXX implement
#define LWD_INVALID_REASON      13 // bounds ting
// used when checking for memory corruption
#define LWD_INVALID_FLAG      0x00 // 0000 0000
#define LWD_RESTART_FLAG      0xA5 // 1010 0101
#define LWD_USER_FLAG         0xA1 // 1010 0001
#define LWD_USER_FLAG_MASK    0xFB // 1111 1011

/* Restart data. Two attributes are defined to control memory allocation and alignment.
 * Members are ordered to ensure struct exactly two RTC memory blocks.
 * The packed attribute is not needed as-is, but if struct members later grow uneven
 * would cause padding to be added to align id on a 4 byte boundary.
 *
 * Second attribute aligns entire struct on a 4 byte boundary, needed by
 * system_rtc_mem_read and system_rtc_write. Not actually needed either as struct
 * contains a 32 bit member and hence will be aligned on a 32 bit boundary.
 *
 * GCC online documentation on attributes: Specifying Attributes of Variables
 * https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Variable-Attributes.html#Variable%20Attributes
 * Espressif, ESP8266 Non-OS SDK API Reference, Version 2.2.1 (2018/05) "3.3.23 system_rtc_mem_write" p. 19
 * https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf#page=1&zoom=auto,-13,792 */
struct __attribute__((packed)) {
  uint8_t       flag; // 1 byte lwdData structure flag
  lwdReason_t reason; // 1 byte lwdData reason
  lwdCount_t   count; // 2 byte (16 bit) count of consecutive boots for same reason
  lwdID_t         id; // 4 byte (32 bit) id (module id or exception cause or user defined)
} lwdData __attribute__((aligned(4)));

#define LWD_RTC_SYSTEM             64
#define LWD_RTC_USER               128
#define LWD_BLOCK_SIZE             ((sizeof(lwdData) + 3) & (~3))
#define LWD_MAX_VALID_RTC_ADDRESS  LWD_RTC_SYSTEM + LWD_RTC_USER - (LWD_BLOCK_SIZE / 4)


class LoopWatchdog {
  Ticker wdt;
  volatile unsigned long fedAt, lapseAt, timeout, negTimeout;
  bool doneGet = false; // Flag to avoid incrementing count more than once.
  uint8_t rtcAddress = LWD_MAX_VALID_RTC_ADDRESS;

  bool validFlag() { return (lwdData.flag & LWD_USER_FLAG_MASK) == LWD_USER_FLAG; }
  void save() { system_rtc_mem_write(rtcAddress, &lwdData, sizeof(lwdData)); }
  void load() {
    system_rtc_mem_read(rtcAddress, &lwdData, sizeof(lwdData));
    if(!validFlag() || lwdData.reason >= LWD_INVALID_REASON)
      initData(); // appeears nothing yet in RTC, try init
  }

  public:
  void feed() { // Called at start of loop: feed and reset loop wdt.
    fedAt = millis();
    lapseAt = fedAt + timeout;
    if(lwdData.id != LWD_ID_LOOP_END) {
      setReason(LWD_REASON_BETWEEN_LOOPS_RST);
      ESP.restart();
    }
  }

  void initData() {
    lwdData.flag = LWD_RESTART_FLAG;
    lwdData.reason = REASON_DEFAULT_RST;
    lwdData.count = 0;
    lwdData.id = LWD_ID_LOOP_END;
  }
  void stamp(lwdID_t id = LWD_ID_LOOP_END) { lwdData.id = id; } //closes loop if no ID passed
  void start(unsigned long timeout = LWD_TIMEOUT); { // called once in setup() to start loop wdt, optionally setting custom timeout interval
    this->timeout = timeout;
    negTimeout = -timeout;
    // ticker.attach_ms(timeout / 2, callback); //why half?
    std::function<void(const LoopWatchdog&)> cb = &LoopWatchdog::callback;
    H4::every(timeout / 2, cb(*this)); //good&rite?
    stamp(LWD_ID_LOOP_END); //= start of loop()
    feed();
  }

  void setReason(lwdReason_t reason, bool sameReason, uint8_t flag = 0) { //sets reason, incrementing count if same, and optionally modifying flag
    // if(reason != lwdData.reason) lwdData.count = 0; // will then be incremented by getRestartReason
    lwdData.count = sameReason? ++lwdData.count: 1; // lwdData.count + 1: 1;
    lwdData.reason = reason;
    lwdData.flag = flag? flag: lwdData.flag; //flag; //LWD_USER_FLAG;
    save();
  }

  lwdReason_t getReason(lwdCount_t& count, lwdID_t& id) { // change to just return a struct instead?
  // lwdData* getInfo() {
    if(!doneGet) {
      load();
      lwdReason_t reason = ESP.getResetInfoPtr()->reason;

      if(lwdData.flag == LWD_USER_FLAG && reason == REASON_SOFT_RESTART)
        reason = lwdData.reason;

      bool sameReason = (reason == lwdData.reason);
      if(reason == REASON_EXCEPTION_RST) {
        lwdID_t exceptionCause = ESP.getResetInfoPtr()->exccause;
        sameReason = sameReason && (exceptionCause == lwdData.id); //not same if different ids = different exceptions
        lwdData.id = exceptionCause;
      }
      // lwdData.count = sameReason? ++lwdData.count: 1; // lwdData.count = sameReason? lwdData.count + 1: 1;
      // lwdData.reason = reason;
      // lwdData.flag = LWD_RESTART_FLAG;
      setReason(reason, sameReason, LWD_RESTART_FLAG); //wat exactly this marker for?
      save();
      doneGet = true;
    }
    id = lwdData.id;
    count = lwdData.count;
    return lwdData.reason;
  }

  void ICACHE_RAM_ATTR callback() {
    if(timeout != lapseAt - fedAt || timeout != -negTimeout || !validFlag()) // something's been overwritten
      setReason(LWD_REASON_CORRUPT);
    else if(millis() - fedAt < timeout) // if(millis() - fedAt < LWD_TIMEOUT) //wrong using define surely, custom timeout lacks effect?
      return; // all good, keep going
    else
      setReason(LWD_REASON_LOOP_RST); // hit watchdog timer...
    ESP.restart(); // timeout or memory correuption - restart
  }

  void userRestart(lwdID_t id = 0, lwdReason_t reason = LWD_REASON_USER_RESTART); {// Does ESP.restart() (default) or ESP.reset(), setting id if > 0.  Count will ignore id, only considering reason
    lwdData.id = id? id: lwdData.id; // only put id if actively set
    setReason(reason, (reason = lwdData.reason), LWD_USER_FLAG);
    if(reason == LWD_REASON_USER_RESTART)    ESP.restart();
    else if(reason == LWD_REASON_USER_RESET) ESP.reset();
  }

  String getResetReasonEx() { // Returns human-readable reset info, like ESP.getResetReason(), but also reporting our five additional reasons.
    char buff[80];
    lwdCount_t count;
    lwdID_t id;
    lwdReason_t restartReason = getRestartReason(count, id);
    // switch(getRestartReason(count, id)) {
    //   case :
    //    break;
    // }
    if(restartReason == REASON_EXCEPTION_RST) {
      snprintf(buff, sizeof(buff), "%s %d, count %d", ESP.getResetReason().c_str(), id, count);
    } else if(restartReason <= REASON_EXT_SYS_RST) {
      snprintf(buff, sizeof(buff), "%s, count %d", ESP.getResetReason().c_str(), count);
    } else if(restartReason == LWD_REASON_USER_RESET) {
      snprintf_P(buff, sizeof(buff), PSTR("User reset, id %d, count %d"), id, count);
    } else if(restartReason == REASON_USER_RESTART) {
      snprintf_P(buff, sizeof(buff), PSTR("User restart, id %d, count %d"), id, count);
    } else if(restartReason == LWD_REASON_LOOP_RST) {
      snprintf_P(buff, sizeof(buff), PSTR("Loop watchdog in %d, count %d"), id, count);
    } else if(restartReason == LWD_REASON_BETWEEN_LOOPS_RST) {
      snprintf_P(buff, sizeof(buff), PSTR("Loop watchdog incomplete in %d, count %d"), id, count);
    } else if(restartReason == LWD_REASON_CORRUPT) {
      snprintf_P(buff, sizeof(buff), PSTR("Loop watchdog overwritten in %d, count %d"), id, count);
    } else {
      return ESP.getResetReason();
    }
    return String(buff);
  }

  // Called before start to change RTC storage address. User memory has 128 4-byte blocks,
  // we use 2, so valid range 0 to 126 (default) Returns false if block invalid. OR XXX block busy by other storage we know of?
  bool setRTCAddress(uint8_t addr = LWD_RTC_ADDRESS) {
    rtcAddress = (addr == LWD_RTC_ADDRESS)? LWD_MAX_VALID_RTC_ADDRESS:
                 ((uint16_t)addr + 64 <= LWD_MAX_VALID_RTC_ADDRESS)? addr + 64:
                   rtcAddress; // don't change if invalid
    return rtcAddress == addr;
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
