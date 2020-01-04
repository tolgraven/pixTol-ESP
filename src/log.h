#pragma once

#include <map>
#include <Arduino.h>
// #include <ANSITerm.h> //not an arduino lib. But def wanna extend shit with ansi oi

#define __pfunc__ __PRETTY_FUNCTION__
//there's also:
// __builtin_FUNCTION()
// __func__   obviously
#define _DEBUG_ __func__, Log::DEBUG
/* #define LOG_DBG(...) */


/* class LogOutput { */
/*    // best if can maybe use Outputter somehow. But bit different. */
/*    // I guess actual physical interface (Serial, MQTT, wrapped in a class used by both RenderStage::Outputter and here..) */
/* }; */

// #ifndef UNIT_TESTING //something like that, redefine logger to simple cout for native testing
class iLog { //prob have basic log then a sub from that with HomieNode, to decouple...
public:
  iLog() { }
	enum Level: int8_t { INVALID = -1, TRACE = 0, DEBUG, INFO, WARNING, ERROR, CRITICAL }; //add an "NONE" at end to disable output fully?
  struct LogOutput {
    String id;
    using logFn = std::function<void*(const String&, const String&, const String&)>;
    logFn fn;
    bool enabled; //or should outside control that instead maybe
    LogOutput(const String& id, logFn fn, bool enabled = true):
      id(id), fn(fn), enabled(enabled) {}
  }; //issue with this cleaner approach is lose the automagic settability of lvl etc but fuckit. Fix asap...
  void initOutput(const String& output) {
    if(output == "Serial") {
      outputs["Serial"] = true;
      /* Serial.begin(SERIAL_BAUD); */ // check whether .begin has already been called etc tho... or can fuck eg Strip
    } else if(output == "Serial1") {
      outputs["Serial1"] = true;
      Serial1.begin(SERIAL_BAUD);
    /* } else if(output == "Homie") { */
    /*   using namespace std::placeholders; */
    /*   homieLogger = new HomieNode("Log", "Logger", std::bind(&Log::handleInput, this, _1, _2, _3)); */
    /*   outputs["MQTT"] = true; */
    /*   for(auto& o: outputs) { */
    /*     homieLogger->advertise(o.first.c_str()).settable(); //ugly lol useless fix */
    /*   } */
    /*   homieLogger->advertise("Level").settable(); */
    }
  }

  //XXX ffs cant have different orders like this. fuck that.
	// void log(const String& text, const Level level = INFO, const String& location = __PRETTY_FUNCTION__) const;
	virtual void log(const String& text, const Level level = INFO, const String& location = "fixmacros") const;
  void ln(const String& text, const Level level, const String& location) const;
  void dbg(const String& text, const String& location = "fixmacros") const;
  void err(const String& text, const String& location = "fixmacros") const;

	void f(const String& location, const Level level, const char *format, ...) const;
	void logf(const String& location, const Level level, const char *format, ...) const;
	void fEvery(uint16_t numCalls, uint8_t id, const String& location, const Level level, const char *format, ...) const;

	bool shouldLog(Level lvl) const { return ((int8_t)lvl >= (int8_t)_level); }
	void setLevel(Level lvl) { if(lvl >= DEBUG && lvl <= CRITICAL) _level = lvl; }
  void setOption(const String& option, const String& value); //use in future


protected:
	Level _level = DEBUG;
  std::vector<LogOutput> destinations;
  std::map<String, bool> outputs; // = {{"Serial", true}};
  // or maybe logger only registers and further ups keep track on where to send text...
  String lvlStr[CRITICAL+1] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
  std::vector<String> whiteList, blackList; // control which locations

	Level convert(const String& lvl) const {
    for(uint8_t ilvl = DEBUG; ilvl <= CRITICAL; ilvl++)
      if(lvl.equalsIgnoreCase(lvlStr[ilvl])) return static_cast<Level>(ilvl);
    return INVALID;
  }
	String convert(const Level lvl) const {
    auto index = constrain(lvl, INVALID, CRITICAL+1);
    if(index >= 0) return lvlStr[index];
    else return "INVALID";
  }
};

//yeah this is dumb. use the attach Outputters idea instead and just reg HomieNode, dont be one...
/* class Log: public iLog, public HomieNode { */
/* class Log: public iLog { */
/*   HomieNode* homieLogger = nullptr; */
/*   HomieNode* status = nullptr; */

/*   public: */
/*   Log(): iLog() { } */

/* 	bool handleInput(const String& property, const HomieRange& range, const String& value); */

/*   /1* void loop() override { *1/ */
/*   /1*   /2* any way we can get it to work before homie init? like so doesnt bring down system... *2/ *1/ */
/*   /1* } *1/ */
/* 	void log(const String& text, const Level level = INFO, const String& location = "fixmacros") const override { */
/*     if(!shouldLog(level)) return; */
/*     iLog::log(text, level, location); */

/*     auto it = outputs.find("MQTT"); */
/*     if(it != outputs.end() && it->second && Homie.isConnected()) { */
/*       String mqttPath = convert(level) + ' ' + location; */
/*       homieLogger->setProperty(mqttPath).setRetained(false).send(text); */
/*     } */
/*   } */
/*   void setStatusNode(HomieNode* statusNode) { status = statusNode; } */
/*   void flushProperty(const String& property, const String& value) { */
/*     if(status) status->setProperty(property).send(value); //new property or new value... */
/*   // temp. Attempt to untangle Homie slightly more. Should be an Outputter or something I guess... */
/*   } */
/* }; */
using Log = iLog;
extern Log lg;
/* extern iLog lg; */

