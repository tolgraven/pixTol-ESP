#pragma once

#include <map>
#include <Homie.h>

#define __pfunc__ __PRETTY_FUNCTION__
#define _DEBUG_ __func__, Log::DEBUG

// #ifndef UNIT_TESTING //something like that, redefine logger to simple cout for native testing
class Log: public HomieNode {
public:
  Log(): serial(true), mqtt(true), HomieNode("Log", "Logger") {
    for(auto s: {"Level", "Serial", "MQTT"}) advertise(s).settable();
  }
	enum Level: int8_t { INVALID = -1, DEBUG = 0, INFO, WARNING, ERROR, CRITICAL }; //add an "NONE" at end to disable output fully?
  // XXX decouple from Homie - inherit a base Log class to HomieLog or something...
	bool handleInput(const String& property, const HomieRange& range, const String& value) override;

	void log(const String& location, const Level level, const String& text) const;
	void logf(const String& location, const Level level, const char *format, ...) const;
	void logfEvery(uint16_t numCalls, const String& id, const String& location, const Level level, const char *format, ...) const;

	bool shouldLog(Level lvl) const { return ((uint8_t)lvl >= (uint8_t)_level); }
	void setLevel(Level lvl) { if(lvl >= DEBUG && lvl <= CRITICAL) _level = lvl; }

private:
  static const Level defaultLevel = DEBUG;
	Level _level;
	bool serial;
  bool mqtt;

	static Level convert(const String& lvl) {
    String lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    for(uint8_t iLevel = DEBUG; iLevel <= CRITICAL; iLevel++)
      if(lvl.equalsIgnoreCase(lvlStr[iLevel])) return static_cast<Level>(iLevel);
    return INVALID;
  }
	static String convert(const Level lvl) {
    String lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    auto index = constrain(lvl, INVALID, CRITICAL+1);
    if(index >= 0) return lvlStr[index];
    else return "INVALID";
  }
};

extern Log lg;
