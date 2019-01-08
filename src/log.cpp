#include "log.h"

bool Log::handleInput(const String& property, const HomieRange& range, const String& value) {
  logf("Log settings", DEBUG, "requested setting property %s to %s", property.c_str(), value.c_str());
  if(property == "Level") {
    Level newLevel = convert(value);
    if(newLevel == INVALID) {
      logf("Log settings", WARNING, "Received invalid level %s.", value.c_str());
      return false;
    }
    _level = newLevel;
    logf("Log settings", INFO, "Level now %s", value.c_str());
    setProperty("Level").send(value);
    return true;
  } else if(property == "Serial") {
    bool on = (value == "ON");
    serial = on;
    logf("Log settings", INFO, "Serial output %s", on? "on": "off");
    setProperty("Serial").send(on? "On": "Off");
    return true;
  }
  logf("Log settings", ERROR, "Invalid property %s, value %s", property.c_str(), value.c_str());
  return false;
}

void Log::log(const String& location, const Level level, const String& text) const {
  if(!shouldLog(level)) return;
  String mqtt_path(convert(level));
  mqtt_path.concat('/');
  mqtt_path.concat(location);
  if(mqtt   && Homie.isConnected())  setProperty(mqtt_path).setRetained(false).send(text);
  if(serial || !Homie.isConnected()) Serial.printf("%d: %s:%s\n", millis(), mqtt_path.c_str(), text.c_str());
}

void Log::logf(const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;
  va_list arg;
  va_start(arg, format);
  char temp[100]; // char* buffer = temp;
  size_t len = vsnprintf(temp, sizeof(temp), format, arg);
  va_end(arg);
  log(location, level, temp);
}

void Log::logfEvery(uint16_t numCalls, const String& id, const String& location, const Level level, const char *format, ...) const {
  if(!shouldLog(level)) return;

  static std::map<String, uint16_t> callTracker;
  callTracker[id] = (callTracker.find(id) != callTracker.end())? callTracker++: 1;
  // if(callTracker.find(id) != callTracker.end()) callTracker[id]++;
  // else callTracker[id] = 1;
  if(callTracker[id] < numCalls) return;
  else callTracker.erase(id); //dunno when will be needed next so might as well remove, rather than zero

  va_list arg;
  va_start(arg, format);
  char temp[100]; // char* buffer = temp;
  size_t len = vsnprintf(temp, sizeof(temp), format, arg);
  va_end(arg);
  log(location, level, temp);
}

Log lg;
