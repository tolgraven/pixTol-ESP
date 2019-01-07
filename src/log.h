#pragma once

#include <map>
#include <Homie.h>
// #include <ANSITerm.h> //not an arduino lib. But def wanna extend shit with ansi oi

#define __pfunc__ __PRETTY_FUNCTION__
//there's also:
// __builtin_FUNCTION()
// __func__   obviously
#define _DEBUG_ __func__, Log::DEBUG


// #ifndef UNIT_TESTING //something like that, redefine logger to simple cout for native testing
// class Log: public HomieNode { //prob have basic log then a sub from that with HomieNode, to decouple...
class iLog { //prob have basic log then a sub from that with HomieNode, to decouple...
public:
  iLog() {
    // outputs["serial"] = Serial.available(); //I guess? maybe?
    outputs["serial"] = true;
    // if(Serial.available())
  }
  // Log(): HomieNode("Log", "Logger") {
  //   for(auto s: {"Level", "Serial", "MQTT"}) advertise(s).settable();
  // }
  // XXX is having enum and converting blabla worth it just to go DEBUG not "DEBUG"? heh
	enum Level: int8_t { INVALID = -1, DEBUG = 0, INFO, WARNING, ERROR, CRITICAL }; //add an "NONE" at end to disable output fully?
  // // XXX decouple from Homie - inherit a base Log class to HomieLog or something...
	// bool handleInput(const String& property, const HomieRange& range, const String& value) override;

	// void log(const String& location, const Level level, const String& text) const;
	// void log(const String& text, const Level level = INFO, const String& location = __PRETTY_FUNCTION__) const;
	virtual void log(const String& text, const Level level = INFO, const String& location = __func__) const;
  void ln(const String& text, const Level level, const String& location) const;
  void dbg(const String& text, const String& location = __func__) const;
  void err(const String& text, const String& location = __func__) const;

	void f(const String& location, const Level level, const char *format, ...) const;
	void logf(const String& location, const Level level, const char *format, ...) const;
	void fEvery(uint16_t numCalls, const String& id, const String& location, const Level level, const char *format, ...) const;

	bool shouldLog(Level lvl) const { return ((uint8_t)lvl >= (uint8_t)_level); }
	void setLevel(Level lvl) { if(lvl >= DEBUG && lvl <= CRITICAL) _level = lvl; }
  void setOption(const String& option, const String& value); //use in future

protected:
	Level _level;
  std::map<String, bool> outputs; // = {{"Serial", true}};
	// bool serial = true; //, mqtt = true;
  // std::map<String, bool> outputs; //more like, i guess. like later will have screen, web...
  // or maybe logger only registers and further ups keep track on where to send text...
  String lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
  // static String lvlStr[CRITICAL+1];

	Level convert(const String& lvl) const {
	// Level convert(String& lvl) {
    // static String lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    for(uint8_t ilvl = DEBUG; ilvl <= CRITICAL; ilvl++)
      if(lvl.equalsIgnoreCase(lvlStr[ilvl])) return static_cast<Level>(ilvl);
    return INVALID;
  }
	String convert(const Level lvl) const {
	// String convert(Level lvl) {
    // static String lvlStr[CRITICAL+1] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    auto index = constrain(lvl, INVALID, CRITICAL+1);
    if(index >= 0) return lvlStr[index];
    else return "INVALID";
  }
};

//yeah this is dumb. use the attach Outputters idea instead and just reg HomieNode, dont be one...
class Log: public iLog, public HomieNode {
  public:
  Log(): iLog(), HomieNode("Log", "Logger") {
    outputs["MQTT"] = true;
    for(auto s: {"Level", "Serial", "MQTT"}) advertise(s).settable();
    // for(auto s: {"Level", outputs.}) advertise(s).settable(); //ugly lol useless fix
  }
  // bool mqtt = true;
  // bool tryProperty(const String& property);
	bool handleInput(const String& property, const HomieRange& range, const String& value) override;

	void log(const String& text, const Level level = INFO, const String& location = __func__) const override {
    if(!shouldLog(level)) return;
    iLog::log(text, level, location);
    String path = convert(level) + ' ' + location;
    if(outputs.at("MQTT")  && Homie.isConnected())
      setProperty(path).setRetained(false).send(text);
  }
};

extern Log lg;
