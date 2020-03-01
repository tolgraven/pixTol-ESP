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


bool Log::initOutput(const String& output) {
  Print* pr = nullptr;
  if(output == "Serial") { // tho, obviously, entire point is being able to define elsewhere.
    // auto fmtFn = [](const String& text, const String& level, const String& location) {
    //   Serial.printf("%lu\t[%s]\t%s\t%s",
    //       millis(), level.c_str(), location.c_str(), text.c_str()); };
    // destinations.emplace_back(output, Serial, fmtFn);
    pr = &Serial;

  } else if(output == "Serial2") {
    Serial1.begin(SERIAL_BAUD);
    // serialObject = &Serial1;
  /* } else if(output == "Homie") { */
  /*   using namespace std::placeholders; */
  /*   homieLogger = new HomieNode("Log", "Logger", std::bind(&Log::handleInput, this, _1, _2, _3)); */
  /*   for(auto& o: outputs) { */
  /*     homieLogger->advertise(o.first.c_str()).settable(); //ugly lol useless fix */
  /*   } */
  /*   homieLogger->advertise("Level").settable(); */
  }
  if(pr) {
    // destinations.emplace_back(output, *pr);
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
  // String lvl = std::move(convert(level));
  // String loc = Ansi<Blue>(location);
  for(auto&& dest: destinations) {
    dest.log(location, lvl, text);
  }
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
void Log::logf(const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;
  va_list arg; va_start(arg, format);
  char b[180];
  size_t len = vsnprintf(b, sizeof(b), format, arg);
  va_end(arg);
  log(b, level, location);
}

void Log::fEvery(uint16_t numCalls, uint8_t id, const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;

  /* static std::map<String, uint16_t> callTracker; */
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


/* bool Log::handleInput(const String& property, const HomieRange& range, const String& value) { */
/*   f("Log settings", DEBUG, "requested setting property %s to %s", property.c_str(), value.c_str()); */
/*   if(property == "Level") { */
/*     Level newLevel = convert(value); */
/*     if(newLevel == INVALID) { */
/*       f("Log settings", ERROR, "Received invalid level %s.", value.c_str()); */
/*       return false; */
/*     } */
/*     _level = newLevel; */
/*     f("Log settings", INFO, "Level now %s", value.c_str()); //this needed tho when setproperty does something right? */
/*     homieLogger->setProperty("Level").send(value); */
/*     return true; */
/*   } else if(property == "Serial" || property == "MQTT") { //check keys rather, ugh */
/*     bool state = (value.equalsIgnoreCase("on") || value == "1"); //or some lib fn for checken positive hapy words or signals heh */
/*     f("Log settings", INFO, "%s output %s", property.c_str(), state? "ON": "OFF"); */
/*     homieLogger->setProperty(property).send(state? "On": "Off"); */
/*     outputs[property] = state; */
/*     // if(property == "Serial") serial = state; */
/*     // if(property == "MQTT") mqtt = state; */
/*     return true; */
/*   } */
/*   f("Log settings", ERROR, "Invalid: %s / %s", property.c_str(), value.c_str()); */
/*   return false; */
/* } */

Log lg;
