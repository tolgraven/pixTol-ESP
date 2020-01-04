#include "log.h"

// terminal.clear_screen();
// terminal.hide_cursor();
// terminal.set_display_style(ANSITerm::SGR_NONE);
// terminal.set_text_colour(ANSITerm::SGR_RED);
// // terminal.draw_box(3,3,19,5,ANSITerm::simple,false);
// terminal.set_text_colour(ANSITerm::SGR_WHITE);
// terminal.set_display_style(ANSITerm::SGR_BOLD);
// terminal.set_cursor_position(5,4);
// terminal.printf("ANSI Terminal");

// iLog::lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};

void iLog::log(const String& text, const Level level, const String& location) const {
  if(!shouldLog(level)) return;
  String path = convert(level) + " / " + location;
  // then ideally like for(auto& out: outputs) out.print(); and if disabled well, wont
  auto it = outputs.find("Serial");
  if(it != outputs.end() && it->second) {
    Serial.printf("%lu  [%s] \t  %s", millis(), path.c_str(), text.c_str());
  }
}
void iLog::ln(const String& text, const Level level, const String& location) const {
  log(text + "\n", level, location);
}
void iLog::dbg(const String& text, const String& location) const {
  ln(text, DEBUG, location);
}
void iLog::err(const String& text, const String& location) const {
  ln(text, ERROR, location);
}

void iLog::f(const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;
  va_list arg; va_start(arg, format);
  char b[100]; //so, if too long, then what?
  size_t len = vsnprintf(b, sizeof(b), format, arg); // just gets trunc presumably?
  va_end(arg);
  log(b, level, location);
}
//deprecated - use f()
void iLog::logf(const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;
  va_list arg; va_start(arg, format);
  char b[100];
  size_t len = vsnprintf(b, sizeof(b), format, arg);
  va_end(arg);
  log(b, level, location);
}

void iLog::fEvery(uint16_t numCalls, uint8_t id, const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;

  /* static std::map<String, uint16_t> callTracker; */
  static std::map<uint8_t, uint16_t> callTracker;
  callTracker[id] = (callTracker.find(id) == callTracker.end())? 1: ++callTracker[id]; //init or inc
  if(callTracker[id] < numCalls) return;
  else callTracker.erase(id); //dunno when will be needed next so might as well remove, rather than zero

  // f(location, level, format, arg); //no work right? :(
  va_list arg; va_start(arg, format);
  char b[100];
  size_t len = vsnprintf(b, sizeof(b), format, arg);
  va_end(arg);
  log(location, level, b);
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
/* iLog lg; */
