// firmware, updates, wifi detail swarm sharing stuff...
#pragma once

#include <vector>

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <Ticker.h>
// #include <NtpClientLib.h>

#include "log.h"
#include "firmware.h"
#include "watchdog.h"
#include "debuglog.h"
#include "io/networking.h"

namespace tol {

enum class Status { //based off homie, wrap that first and extend with eg RDM, OPC, HomeKit etc status events
  Invalid = -1, Setup = 1, Normal,
  OtaStart, OtaTick, OtaSuccess, OtaFailure,
  WiFiConnect, WiFiDisconnect,
  MqttConnect, MqttDisconnect, MqttPacket,
  PrepareReset, PrepareSleep
};

enum class DeviceStatus { //based off homie, wrap that first and extend with eg RDM, OPC, HomeKit etc status events
  Invalid = -1, Boot = 1, Normal, Config, FirmwareUpdate, // then subevt OtaStart, OtaTick, OtaSuccess, OtaFailure,
  PrepareRestart, PrepareSleep
};
enum class EventType { // based on OTA stuff but could be used for lots of stuff?
  // Invalid = -1, Start = 1, Progress, Success, Error, Custom, Unknown
  Invalid = -1, Start = 1, Progress, End, Custom, Unknown
};
enum class EventResult {
  Invalid = -1, Success = 0, Error // and then error codes all the way down?
};
struct BasicEvent {
  DeviceStatus location;
  EventType type;
  EventResult error;
  uint32_t time; // also want NTP actual time I guess. tho RTC drift and blabla
};
// urr anyways mod so basically like homies events, except scheduled not interrupt.
// so events arent handled until next pass through main loop
//
// then again no reason to push too far that way and not migrate to 32 and actual threads&scheduling lol
// see eg ping goes bananas when letting Artnet out go full tilt...
//

// while really a million ways for signals and tho resolution is lol, midi is still the king of easy control input heh.
// also just for generating, tap a synth. record shit really all my early really exciting "fuck building just repurpose" ideas just...  fell off.
// I was hooking Live up to my fucking modded milights five years ago through fucking node-red and it worked fuckng badass!!
// ffs take all this knowledge and build some practical and actually usable shit.  show it to people.
class Device { //inits hardware stuff, holds Updaters, PhysicalUI/components, more?
  private:
  String _id;
  std::map<String, Updater*> updater;
  // NTPTime time; // reckon will help with the rougher (simpler) parts of sync https://github.com/aharshac/EasyNTPClient
  /* PhysicalUI* ui = nullptr; */ // WebUI* webUI;
  // Status lastStatus;
  Ticker recurring;
  // std::unique_ptr<WifiConnection> wifi;
  // bool connected = false;
  // FileIO fs;
  // String cfgFile = "/config.json"; //let's just use WifiManager quick while then move to ConfigManager
  public:

  Debug* debug = nullptr;
  uint32_t timeBootDone, timeCurrent;
  /* Config* config; //we'll need to make one of these. */

  Device(const String& id): _id(id) { // serial, random seed mean this before anything else but that makes sense since want watchdog, statusled, display etc from boot...
    // uint8_t stackStart = 0;
    // debug = new Debug(&stackStart);
    // Serial.begin(SERIAL_BAUD); // XXX tempwell, not if uart strip tho...
    // lg.initOutput("Serial");  //logging can be done after here
    Serial2.begin(SERIAL_BAUD, SERIAL_8N1, 4, 2);
    lg.initOutput("Serial2");  //mirror to pins 2-4

    uint32_t finishAt = millis() + 2000;
    auto dotter = [finishAt]() {
                    if(millis() < finishAt) {
                      Serial.print("."); return true;
                    } else return false;
                  };
    recurring.attach_ms(250, dotter); // so, cant find where in lib return false to detach actually handled?

    lwd.init(); //muy important, chilled out constructor
    lg.dbg(lwd.getResetReason() + (lwd.softResettable()? ", softResettable!": ""));

    // lg.dbg(debug->getStacktrace());
    // OTA update leads to crash in Neo i2s_slc_isr
    // SaveCrash leads to stack overflow repeat exception while handling exception
    // debug->saveCrash.print(lg); //can use our logger as output. i thought?
    // debug->saveCrash.print(); //can use our logger as output. i thought?
    // if(debug->saveCrash.count() >= 3) debug->saveCrash.clear();


    initRNG();

    // wifi = std::make_unique<WifiConnection>("pixTol-proto", "wifi");
    // connected = wifi->start(); //blocking...

    // updater["OTA"] = new ArduinoOTAUpdater(_id);
    /* updater["HTTP"] = new HttpUpdater(String(Homie.getConfiguration().mqtt.server.host), 1880, s); */

    handleBootLoop();

    /* lwd.stamp(PIXTOL_SETUP_EXIT); */
    timeBootDone = micros();

    lg.dbg("Done Device");
  }

  ~Device() { updater.clear(); }

  void handleBootLoop() {
    auto resetCount = lwd.getCount();
    if(resetCount >= 3 && resetCount % 2 > 0) { // this also when we can take the time to print the traces I guess, and clear...
      lg.dbg("\n\nSAVE YOURSELF\n\n");
      // debug->saveCrash.print();
      // debug->saveCrash.clear();

      // disable helpfully actual culprit but main is just ensure updater lives would still make sense just stalling here tho still feeding wdt giving chance to update...
      uint32_t otaTimeout = constrain(resetCount * 3000, 3000, 30000);
      uint32_t finishAt = millis() + otaTimeout;
      auto dotter = [finishAt]() {
                      if(millis() < finishAt) {
                        Serial.print("."); return true;
                      } else return false; };
      // recurring.attach_scheduled(1, dotter); // oh yeah bc we never reach loop, no shit.
      recurring.attach(1, dotter); // so, cant find where in lib return false to detach actually handled?  nor where I read about it. hmm.
      while(millis() < finishAt) {
        lwd.feed();
        this->loop();
        lwd.stamp();
#ifdef ESP8266
        ESP.wdtFeed();
#endif
      }
    }
  }


  void loop() {
    for(auto& up: updater)
      up.second->loop();
  }

  void initRNG() { // slightly less shit-resolution analogRead random seed ting...
    uint32_t seed = analogRead(0); // randomSeed(seed); //multiple seeding seems to krax??
    delayMicroseconds(190);
    for(int shifts = 3; shifts < 31; shifts += 3) {
      seed ^= analogRead(0) << shifts; // seed ^= rand() * analogRead(0) << shifts;
      delayMicroseconds(200); // delayMicroseconds(200 + rand());
    } randomSeed(seed);
  }

};



class SystemComponent {
  public:
  SystemComponent() {}
  void loop() {}
};

class PowerBudget {
  // had some minor power stuff in the battery experiment but so many aspects.
  // wearable/battery etc can limit to target much better longetivity,
  // but smaller stuff (USB etc!) also have massive peak power budget
  // and working with that can run p loads of stuff off little (+ even better with bunch big caps???)
  // just by generally dim and high lum v local and transient.
  //
  // Question is how you enforce that in a nice lookng way.
  // But basic thing (fastled does it?) we know about how much each LED uses and what is active...
  //
  // so put one in an Outputter run the numbers adding up each frames total.
  // and just ev run into compression/deminishing returns.
  //
  // more effect wise remember old ideas for afterglow with slow compressor making peaks stand out etc
};
class DataIO: public Named { // something common then subclass for SPIFFS, EEPROM, RTC... different use cases
  public:
  DataIO(): Named() {} // or duh use c++ ffs you dummy. fstream...
  virtual ~DataIO() {}

  virtual bool start() = 0;
  virtual bool stop() = 0;
  virtual bool clear() = 0;
  virtual bool write(const String& location, const String& contents) = 0;
  virtual std::unique_ptr<char[]> read(const String& location) = 0; // virtual char* read() = 0;
  // bool writeConfig(); // SPIFFS, RTC, EEPROM..
  // void readConfig();
};
// class FileIO: public DataIO {
//   public:
//   FileIO(): DataIO() {}
//   virtual ~FileIO() { stop(); }

//   virtual bool start() { return SPIFFS.begin(); } // or even skip theses and enough w ctor/dtor...
//   virtual bool stop() { SPIFFS.end(); return true; }
//   virtual bool clear() { return SPIFFS.format(); }
//   virtual bool write(const String& path, const String& contents) {
//     File file = SPIFFS.open(path, "w"); //whether or not file exists we can write to it...
//     if(!file) { return false; }
//     file.print(contents);
//     file.close();
//     return true;
//   }

//   // virtual char* read(const String& path) {
//   virtual std::unique_ptr<char[]> read(const String& path) {
//     if(!SPIFFS.exists(path)) return nullptr;
//     File file = SPIFFS.open(path, "r"); //file exists, reading and loading
//     if(!file)  return nullptr;  // for long like these maybe a helper fn that logs an error and returns... or statuscodes.

//     size_t size = file.size();
    // std::unique_ptr<char[]> buf(new char[size]); // Allocate a buffer to store contents of the file.
    // file.readBytes(buf.get(), size);

    // return buf;
  // }
// };
// class EepromIO: public DataIO { //raw flash writes tho like, eh, need?
//   size_t size;
//   public:
//   EepromIO(size_t size = 512): DataIO(), size(size) { }
//   virtual ~EepromIO() { stop(); }
//   virtual bool start() { EEPROM.begin(size); }
//   virtual bool stop() { EEPROM.end(); }
//   virtual bool write(const String& fileName, const String& contents) { }
//   virtual std::unique_ptr<char[]> read(const String& fileName) { }
// };
// class DataFormat { //to encode as json bubt also binary packing crap yada
//   DataFormat() {}
//   ~DataFormat() {}
// };

}
