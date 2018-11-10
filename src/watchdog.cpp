#include <Ticker.h>
#include "watchdog.h"

// extern "C" {
// #include "user_interface.h"
// }

// bool softResettable() {
//   int bootmode = 0;
//   asm ("movi %0, 0x60000200\n\t"
//        "l32i %0, %0, 0x118\n\t"
//        : "+r" (bootmode) /* Output */
//        : /* Inputs (none) */
//        : "memory" /* Clobbered */ );
//   return (((bootmode >> 0x10) & 0x7) != 1);
// }

/* Struct of restart information. Two attributes are defined to control
 * memory allocation and alignment.
 * Members are ordered to ensure struct occupies exactly two blocks.
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
// struct __attribute__((packed)) {
//   uint8_t           flag; // 1 byte restart structure flag
//   restartReason_t reason; // 1 byte restart reason
//   restartCount_t   count; // 2 byte (16 bit) count of consecutive boots for same reason
//   restartID_t         id; // 4 byte (32 bit) id (module id or exception cause or user defined)
// } restart __attribute__((aligned(4)));


// uint8_t rtcAddress = LAST_VALID_BLOCK_ADDRESS;

// bool setRTCAddress(uint8_t addr) {
//   bool result = true;
//   if(addr == DEFAULT_RESTART_ADDRESS) {
//     rtcAddress = LAST_VALID_BLOCK_ADDRESS;
//   } else {
//     addr = addr + 64;
//     result = (addr <= LAST_VALID_BLOCK_ADDRESS);
//     if(result) rtcAddress = addr;
//   }
//   return result;
// }

// void initRestart() {
//   restart.flag = LWD_RESTART_MARKER;
//   restart.reason = 0;
//   restart.count = 0;
//   restart.id = LOOP_END;
// }

// void loadRestart() {
//   system_rtc_mem_read(rtcAddress, &restart, sizeof(restart));
//   if((restart.flag & MARKER_MASK) != LWD_USR_MARKER ||
//       restart.reason > LAST_VALID_REASON)
//     initRestart();
// }

// void saveRestart() { system_rtc_mem_write(rtcAddress, &restart, sizeof(restart)); }

// void setLwdtRestartReason(restartReason_t reason) {
//   if(restart.reason != reason) restart.count = 0;  // will then be incremented by getRestartReason
//   restart.reason = reason;
//   restart.flag = LWD_USR_MARKER;
//   saveRestart();
// }

// bool restartUpdated = false; // Flag to avoid incrementing count more than once.

// restartReason_t getRestartReason(restartCount_t &count, restartID_t &id) {
//   if(!doneUpdate) {
//     loadRestart();
//     restartReason_t reason = ESP.getResetInfoPtr()->reason;
//
//     if((restart.flag == LWD_USR_MARKER) && (reason == REASON_SOFT_RESTART))
//       reason = restart.reason;
//
//     bool sameReason = (reason == restart.reason);
//     if(reason == REASON_EXCEPTION_RST) {
//       restartID_t exceptionCause = ESP.getResetInfoPtr()->exccause;
//       sameReason = sameReason && (exceptionCause == restart.id);
//       restart.id = exceptionCause;
//     }
//
//     restart.count = sameReason? restart.count + 1: 1;
//     restart.flag = LWD_RESTART_MARKER;
//     restart.reason = reason;
//     saveRestart();
//     doneUpdate = true;
//   }
//   id = restart.id;
//   count = restart.count;
//   return restart.reason;
// }

// void userRestart(restartID_t id = 0, restartReason_t method = REASON_USER_RESTART) {
//   restart.id = id? id: restart.id; // only put id if actively set
//   setLwdtRestartReason(method);
//   if(method == REASON_USER_RESTART)    ESP.restart();
//   else if(method == REASON_USER_RESET) ESP.reset();
// }

// String getResetReasonEx() {
//   char buff[80];
//   restartCount_t count;
//   restartID_t id;
//   restartReason_t restartReason = getRestartReason(count, id);
//   if(restartReason == REASON_EXCEPTION_RST) {
//     snprintf(buff, sizeof(buff), "%s %d, count %d", ESP.getResetReason().c_str(), id, count);
//   } else if(restartReason <= REASON_EXT_SYS_RST) {
//     snprintf(buff, sizeof(buff), "%s, count %d", ESP.getResetReason().c_str(), count);
//   } else if(restartReason == REASON_USER_RESET) {
//     snprintf_P(buff, sizeof(buff), PSTR("User reset, id %d, count %d"), id, count);
//   } else if(restartReason == REASON_USER_RESTART) {
//     snprintf_P(buff, sizeof(buff), PSTR("User restart, id %d, count %d"), id, count);
//   } else if(restartReason == REASON_LWD_RST) {
//     snprintf_P(buff, sizeof(buff), PSTR("Loop watchdog in %d, count %d"), id, count);
//   } else if(restartReason == REASON_LWD_LOOP_RST) {
//     snprintf_P(buff, sizeof(buff), PSTR("Loop watchdog incomplete in %d, count %d"), id, count);
//   } else if(restartReason == REASON_LWD_OVW_RST) {
//     snprintf_P(buff, sizeof(buff), PSTR("Loop watchdog overwritten in %d, count %d"), id, count);
//   } else {
//     return ESP.getResetReason();
//   }
//   return String(buff);
// }

// Ticker lwdTicker;
// volatile unsigned long lwdTime;
// volatile unsigned long lwdTimeout;
// volatile unsigned long lwdDifference;
// volatile unsigned long lwdNotDifference;

// void lwdtStamp(restartID_t id) {
//   restart.id = (restartID_t) id;
// }

// void lwdtFeed() {
//   lwdTime = millis();
//   lwdTimeout = lwdTime + lwdDifference;
//   if (restart.id != LOOP_END) {
//     setLwdtRestartReason(REASON_LWD_LOOP_RST);
//     ESP.restart();
//   }
// }

// void ICACHE_RAM_ATTR lwdtcb() {
//   if(millis() - lwdTime < LWD_TIMEOUT)
//     return; // all good, keep going
//   else if(lwdDifference != lwdTimeout - lwdTime ||
//           lwdDifference != -lwdNotDifference    ||
//          (restart.flag & MARKER_MASK) != LWD_USR_MARKER) // where variable mangled
//     setLwdtRestartReason(REASON_LWD_OVW_RST);
//   else
//     setLwdtRestartReason(REASON_LWD_RST);
//   ESP.restart();   // lwd timed out or lwd overwritten
// }

// void lwdtInit(unsigned long timeout) {
//   lwdNotDifference = -timeout;
//   lwdDifference = timeout;
//   lwdTicker.attach_ms(timeout / 2, lwdtcb);
//   lwdtStamp(LOOP_END); //= start of loop()
//   lwdtFeed();
// }
