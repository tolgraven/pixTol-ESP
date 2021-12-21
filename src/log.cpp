#include "log.h"

Print* printer; // gets set to Log. which appropriately forwards to the Print objs within.

extern "C" {
int _write(int file, char *ptr, int len) {
  (void) file; // guess it's 0-1-2 but uh do we do anything with it?
  if(printer) {
    return printer->write(ptr, len);
  } else
    return 0;
}
}

namespace tol {

bool Log::initOutput(const String& output) {
  Print* pr = nullptr;
  if(output == "Serial") { // tho, obviously, entire point is being able to define elsewhere.
    pr = &Serial;
  } else if(output == "Serial1") {
    pr = &Serial1;
  } else if(output == "Serial2") {
    pr = &Serial2;
  }
  if(pr) {
    auto dest = LogOutput(output, *pr);
    if(enableColor) {
      auto fmt = [](const String& loc, const String& lvl, const String& txt) {
                  return String((String)millis() +
                      "\t[" + Ansi<Yellow>(lvl) + "]\t" +
                      Ansi<Blue>(loc) + "\t" + txt); };
      dest.setFmt(fmt);
    }
    destinations.push_back(dest);
    ln("Logging started", INFO, output);
  } else {
    ln("Failed to add output", ERROR, output);
  }
  return pr;
}

void Log::log(const String& text, const Level level, const String& location) const {
  if(!shouldLog(level)) return;
  if(whiteList.size() > 0) { // filter-in active
    bool found = false;
    for(auto& s: whiteList) {
      if(s == location) {
        found = true;
        break;
      }
    }
    if(!found) return;
  } else if(blackList.size() > 0) {// filter out
    for(auto& s: blackList) {
      if(location == s) return;
    }
  }

  String lvl = convert(level);
  auto& guard = smooth::core::logging::Log::guard;
  std::unique_lock<std::mutex> lock(guard);
  for(auto& dest: destinations) { dest.log(location, lvl, text); }
  for(auto& dest: moreDest) { dest.log(location, lvl, text); } // temp. need tuuplay of various strategies
}

void Log::ln(const String& text, const Level level, const String& location) const {
  log(text + "\n", level, location);
}
void Log::dbg(const String& text, const String& location) const {
  ln(text, DEBUG, location);
}
void Log::err(const String& text, const String& location) const {
  ln(text, ERROR, location);
}

void Log::f(const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;
  va_list arg; va_start(arg, format);
  char b[180]; //so, if too long, then what? fooled me once!
  size_t len = vsnprintf(b, sizeof(b), format, arg); // just gets trunc presumably?
  va_end(arg);
  log(b, level, location);
}

void Log::fEvery(uint16_t numCalls, uint8_t id, const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;

  static std::map<uint8_t, uint16_t> callTracker;
  callTracker[id] = (callTracker.find(id) == callTracker.end())? 1: ++callTracker[id]; //init or inc
  if(callTracker[id] < numCalls) return;
  else callTracker.erase(id); //dunno when will be needed next so might as well remove, rather than zero

  // f(location, level, format, arg); //no work right? :(
  va_list arg; va_start(arg, format);
  char b[180];
  size_t len = vsnprintf(b, sizeof(b), format, arg);
  va_end(arg);
  log(b, level, location);
}


Log lg;
}
