#pragma once

#undef max
#undef min
#include <stdio.h>
#include <map>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <Arduino.h>

#include <smooth/core/logging/log.h>
// #include "fmt/format.h"
// #include <Print.h> // how design to implement Print stuff yet include log shit w/o knowing bout it..
// #include <PrintStream.h>


class Print;

extern Print* printer; // gets used by _write, so stay dynamic.

extern "C" {
int _write(int file, char *ptr, int len); // redefine for GCC to autoredirect buncha stuff. why not case by default btw??
}
// available:
// int _read(int file, char *data, int len)
// int _close(int file)
// int _lseek(int file, int ptr, int dir)
// int _fstat(int file, struct stat *st)
// int _isatty(int file)

// template<class T>
// inline Print& operator <<(Print&& pr, T&& arg) { pr.print(arg); return pr; }
// inline Print& operator <<(Print&& pr, _EndLineCode arg) { pr.println(); return pr; }
// typedef decltype(endl) endl_t;
// inline Print& operator <<(Print&& pr, endl_t arg) { pr.println(); return pr; }

// doesnt actually make sense since just does same thing as +. just to test...
// template<class T>
// inline String operator <<(String&& s, T&& arg) { return s + arg; }
// // inline String& operator <<(String& s, T&& arg) { s += arg; return s; } // like dis?
// // typedef decltype(endl) endl_t; // why does this work here but not for Print?
// enum _EndLineCode { endl };
// inline String operator <<(String&& s, _EndLineCode arg) { return s + '\n'; }

namespace tol {

enum AnsiColor: int {
    Black=30, Red, Green, Yellow, Blue, Magenta, Cyan, White,
    brBlack=90, brRed, brGreen, brYellow, brBlue, brMagenta, brCyan, brWhite
};

template<typename T>
String Ansi(T& value, AnsiColor color) {
    return (String)"\033[1;" + static_cast<int>(color) + "m" + value + "\033[0m";
}
template<AnsiColor color, typename T>
String Ansi(T& value) {
    return (String)"\033[1;" + static_cast<int>(color) + "m" + value + "\033[0m";
}

namespace ansi {
  // template <class CharT, class Traits>
  // using ansistream = std::basic_ostream<CharT, Traits>; // can do that if using dummy struct not ns

  template<class CharT, class Traits> constexpr
  std::basic_ostream<CharT, Traits>& reset(std::basic_ostream<CharT, Traits>& os) {
     return os << "\033[0m";
  }
  template<class CharT> constexpr
  std::basic_ostream<CharT>& fgRed(std::basic_ostream<CharT>& os) {
     return os << "\033[1m\033[31m";
  }
  template<AnsiColor code, class CharT> constexpr
  std::basic_ostream<CharT>& color(std::basic_ostream<CharT>& os) {
     return os << "\033[1;" << static_cast<int>(code) << "m";
  }

  // template <class CharT> // or build a builder? no I wuz silly ha
  // std::function<std::basic_ostream<CharT>& (std::basic_ostream<CharT>&)>
  // // create_ansi(const std::string& ansiCode) {
  // create_ansi(const String& ansiCode) {
  //   return [ansiCode](std::basic_ostream<CharT>& os) { return os << ansiCode; };
  // }
  // auto red = create_ansi<char>(aRED);
}

#define __LOC__ (String)__func__ + " " + __FILE__ + ":" + __LINE__
#define TRACE(s) tol::lg.ln(s, tol::Log::TRACE, __LOC__)
// #define DEBUG(s) tol::lg.ln(s, tol::Log::DEBUG, __func__)
#define DEBUG(s) tol::lg.ln(s, tol::Log::DEBUG, __PRETTY_FUNCTION__)
#define DEBUGF(s, ...) tol::lg.f(__LOC__, tol::Log::DEBUG, s, __VA_ARGS__)
#define ERROR(s) tol::lg.ln(s, tol::Log::ERROR, __LOC__)
#define LOG(s) tol::lg.ln(s, tol::Log::INFO, __func__)


template<class T>   // Dummy parameter-pack expander
void expand(std::initializer_list<T>) {}

// Call a function and log its arguments and return value, fancy version.
// BUT needs bind for member fns uhhh. so either macro or wrap lambdas?
// or become templating ninja and figure out an easier way...
template<class Fun, class... Args>
typename std::result_of<Fun&&(Args&&...)>::type
_logging(Fun&& f, String&& name, Args&&... args) {
    // std::cout << ": ";
    // expand({(std::cout << args << ' ', 0)...});
    // std::cout << endl;
    if(printer) { // print out all args.
      printer->print("calling fn " + name + "(");
      printer->print((String)args + ", " ...);
      printer->print(")");
    } // if the _write thing actually works won't need the check or pointer right? straight printf or cout will work?
    // soo does this work? nope already checked just realized. bc template + dynamic binding...
    // print("calling fn " + name + "("); print((String)args + ", " ...); print(")");

    // forward the call, and log the result.
    auto&& result = std::forward<Fun>(f)(std::forward<Args>(args)...);
    if(printer) printer->println((result != nullptr)? (" -> " + (String)result): "");
    return result;
}
#define logging(f, ...) _logging(f, (String)#f, __VA_ARGS__)
// makes a str out of fn name, then hopefully rest of args also Stringifyable.

class LogOutputClean { // guess concept of both being and containing (and/or either way? a printer seems snazzy)
  using FmtFn = std::function<String(const String&, const String&, const String&)>;
  using PrintFn = std::function<void(const String&)>;
  public:
  LogOutputClean(const String& id, PrintFn printFn, FmtFn fmt, bool enabled = true):
    printFn(printFn), enabled(enabled), fmt(fmt) {} // id(id), pr(reinterpret_cast<Print*>(&printer)), enabled(enabled) {}

  PrintFn printFn;
  bool enabled; //or should outside control that instead maybe

  void log(const String& location, const String& level, const String& text) const {
    if(enabled) log(fmt(location, level, text));
  }
  void log(const String& text) const {
    if(enabled) printFn(text);
  }
  void setFmt(const FmtFn& fmtFn) { fmt = std::move(fmtFn); }

  private:
  FmtFn fmt = [](const String& loc, const String& lvl, const String& txt) {
                  return String((String)millis() + "\t[" + lvl + "]\t" + loc + "\t" + txt); };
}; // YADA YADA what think now:
// - keep arduino logging output
// + add idf output (by way of smooth? for thread-saef)
//    CHALLENGE: keep implementations away ffs
//               (irrelevant while using String but that will change too)
//               (dont just fucking switch from String to std::string, templatize.)
// - add back mqtt out already ffs - smooth has something too

class LogOutput { // guess concept of both being and containing (and/or either way? a printer seems snazzy)
  using FmtFn = std::function<String(const String&, const String&, const String&)>;
  FmtFn fmt = [](const String& loc, const String& lvl, const String& txt) {
                  return String((String)millis() + "\t[" + lvl + "]\t" + loc + "\t" + txt); };
  public:
  Print* pr = nullptr; // Stream* s  // not atm but thinking pr not required. Couldl bind that to self and use other fn.

  void log(const String& location, const String& level, const String& text) const {
    if(enabled) log(fmt(location, level, text));
  }
  void log(const String& text) const { if(enabled) pr->print(text); }
  // size_t printTo(const Print& p) const override {
  //   return Named::printTo(p) + p.println((String)"put fmtString here n shit"); // uh guess print FmtFn text cause should be printf style anyways :| and ze kinda Print.
  // }

  bool enabled; //or should outside control that instead maybe

  LogOutput(const String& id, Print& printer, bool enabled = true):
    //  fmt(std::move(f)) // not sure if inline-default declated lambda w ::move was issue? anyways no biggie
   pr(&printer), enabled(enabled) {} // id(id), pr(reinterpret_cast<Print*>(&printer)), enabled(enabled) {}

  void setFmt(const FmtFn& fmtFn) { fmt = std::move(fmtFn); }
};


class Log: public Print { // gives same functionality as Serial etc, but routing to multiple/different destinations...
public:
  Log() {
    printer = this; // bind as global default for _write (see definition in cpp)
  } // sexiest use of globals since ya
	enum Level: int8_t { INVALID = -1, TRACE = 0, DEBUG, INFO, WARNING, ERROR, CRITICAL }; //add an "NONE" at end to disable output fully?

  bool initOutput(const String& output); // init a predefined
  void addOutput(const LogOutput& lo);   // add new

	void log(const String& text, const Level level = INFO, const String& location = "") const;
  template<Level level>
	void log(const String& text, const String& location = "") const {
    log(text, level, location);
  }
  
  // void ln(const std::string& text, const Level level, const std::string& location) const {
  //   ln((String)text.c_str(), level, (String)location.c_str());
  // }
  // void dbg(const std::string& text, const std::string& location = "") const {
  //   dbg((String)text.c_str(), (String)location.c_str());
  // }
  // void err(const std::string& text, const std::string& location = "") const {
  //   err((String)text.c_str(), (String)location.c_str());
  // }

  
  void ln(const String& text, const Level level, const String& location) const;
  void dbg(const String& text, const String& location = "") const;
  void err(const String& text, const String& location = "") const;

	void f(const String& location, const Level level, const char *format, ...) const;
	void logf(const String& location, const Level level, const char *format, ...) const  __attribute__((deprecated));

	void fEvery(uint16_t numCalls, uint8_t id, const String& location, const Level level, const char *format, ...) const;

	void setLevel(Level lvl) { if(lvl >= DEBUG && lvl <= CRITICAL) _level = lvl; }
  // void setOption(const String& option, const String& value); //use in future

  virtual size_t write(uint8_t character) { // oh yeah this is what complicates... only for arduino stuff otherswise get called in main printer
    for(auto&& d: destinations)
      d.pr->write(character);
    return 1;
  }
  virtual size_t write(const uint8_t* buffer, size_t size) {
    size_t written = 0;
    for(auto&& d: destinations)
      written += d.pr->write(buffer, size);
    return written;
  }

  bool enableColor = true;

  std::vector<LogOutputClean> moreDest; // fix up. need flebility uh
private:
	Level _level = DEBUG;
  std::vector<LogOutput> destinations;

  String lvlStr[CRITICAL+1] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
  std::vector<String> whiteList, blackList; // control which locations. bound to be idiotically slow

	Level convert(const String& lvl) const {
    for(int8_t ilvl = TRACE; ilvl <= CRITICAL; ilvl++)
      if(lvl.equalsIgnoreCase(lvlStr[ilvl]))
        return (Level)ilvl;
    return INVALID;
  }
	String convert(const Level lvl) const {
    auto index = constrain(lvl, INVALID, CRITICAL);
    if(index >= 0) return lvlStr[index];
    else return "INVALID";
  }
	bool shouldLog(Level lvl) const { return ((int8_t)lvl >= (int8_t)_level); }
};

extern Log lg;

template<class... Args>
void DEBUG2(const Args&&... args) {
    // expand({(String << args << ' ', 0)...});
  lg.dbg(String((String)args + ", " ...));
  // lg.dbg(expand({(String << args << ' ', 0)...}));
}

}
