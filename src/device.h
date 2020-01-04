// firmware, updates, wifi detail swarm sharing stuff...
#pragma once

#include <Arduino.h> //needed by linters
/* #include <Homie.h> */

/* #include <WifiEspNowBroadcast.h> */
/* #include <esp_now.h> */

/* #include "config.h" */
/* #include "io/unit.h" */
#include "log.h"
#include "firmware.h"
#include "watchdog.h"
#include "debuglog.h"

enum class Status { //based off homie, wrap that first and extend with eg RDM, OPC, HomeKit etc status events
  Invalid = -1, Setup = 1, Normal,
  OtaStart, OtaTick, OtaSuccess, OtaFailure,
  WiFiConnect, WiFiDisconnect,
  MqttConnect, MqttDisconnect, MqttPacket,
  PrepareReset, PrepareSleep
};

class Device { //inits hardware stuff, holds Updater, PhysicalUI/components, more?
private:
  String _id;
  std::map<String, Updater*> updater;
  // LoopWatchdog* lwd = nullptr;
  /* PhysicalUI* ui = nullptr; */
  // WebUI* webUI;
  Status lastStatus;
public:

  Debug* debug = nullptr;
  uint32_t timeBootDone, timeCurrent;
  /* Config* config; //we'll need to make one of these. */

  Device() { // serial, random seed mean this before anything else but that makes sense since want watchdog, statusled, display etc from boot...
    uint8_t stackStart = 0;
    debug = new Debug(&stackStart);
    Serial.begin(SERIAL_BAUD); //well, not if uart strip tho...
    lg.initOutput("Serial");
    initRNG();

    updater["OTA"] = new ArduinoOTAUpdater(_id);
    /* updater["HTTP"] = new HttpUpdater(String(Homie.getConfiguration().mqtt.server.host), 1880, s); */
    // EEPROM.begin(512); SPIFFS.begin();
    // lg.dbg("Done Device");
  }
  ~Device() {
    updater.clear();
  }
  void finalizeBoot() { // log time etc... what else?
    /* lwd.stamp(PIXTOL_SETUP_EXIT); */
    timeBootDone = micros();
    lg.dbg("Reset reason: " + lwd.getResetReason() + ", softResettable? "
        + (lwd.softResettable()? "YES": "NOT"));
    lg.ln("Setup complete, handing over to main loop...\n", Log::INFO, "Boot");
  }

  bool writeFile();
  void readFile();
  bool writeRTC();
  void readRTC();
  virtual void platformSpecific() {
    /* lg.f(__func__, Log::INFO, "Not implemented. Here go esp32/66/whatever specific things."); */
  } // export $PIOPLATFORM from .ini as PLATFORM_NAME

  void loop() {
    timeCurrent = micros();
    for(auto& up: updater) up.second->loop(); //would fit better here than in "IOT" (shit name) anyways?
    // lwd.feed(); //if so device loop must go first in loop
  }

  void initRNG() { // slightly less shit-resolution analogRead random seed ting...
    uint32_t seed = analogRead(0); // randomSeed(seed); //multiple seeding seems to krax??
    delayMicroseconds(190);
    for(int shifts = 3; shifts < 31; shifts += 3) {
      seed ^= analogRead(0) << shifts; // seed ^= rand() * analogRead(0) << shifts;
      delayMicroseconds(200); // delayMicroseconds(200 + rand());
    } randomSeed(seed);
  }

  void tryWifi() {
    static uint8_t maxAPs = 5;
    /* bool goStation = false; */
    bool connected = false;
    wifi_station_ap_number_set(maxAPs); //max number of saved APs
    struct station_config config[maxAPs];
    wifi_station_get_ap_info(config);

    uint8_t lastId = wifi_station_get_current_ap_id(); //which index
    // on connect, then save to spiffs? but hopefully works disconnected, sop can just go down index...
    while(!connected) {
      lastId++;
      if(lastId >= maxAPs) {
        //first check spiffs for further shit
        /* goStation = true; */
        break;
      }
      wifi_station_ap_change(lastId);
      // try connection etc
      // inbetween each, also look for (hardcoded?) mesh leader station, whether it's got news
    }
  }
  void restart() {
    //should do it good way, through lwd etc...
  }
};

class SystemComponent {
  public:
  SystemComponent() {}
  void loop() {}
};

#define ESP_NOW_MAX_PEERS 20
/* class Swarm: public SystemComponent { */
/*   public: */
/*   bool master = false; // should be possible for any device to be master once it gets creds */
/*   Swarm() { */
/*     // should be created dynamically when not getting online and yada. Or is online + requested */
/*     // then register self ya? */
/*     // mod WifiEspNow to not have global */

/*   } */
/*   static void onPacket(const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg) { */

/*   } */
/*   void broadcastWifiCredentials() { //look up the encryption thing. but will obviously need some kind of pre-shared secret.. */
/*     WifiEspNowPeerInfo peers[ESP_NOW_MAX_PEERS]; */
/*     int nPeers = std::min(WifiEspNow.listPeers(peers, ESP_NOW_MAX_PEERS), ESP_NOW_MAX_PEERS); */
/*     for (int i = 0; i < nPeers; ++i) { */
/*       Serial.printf(" %02X:%02X:%02X:%02X:%02X:%02X", peers[i].mac[0], peers[i].mac[1], peers[i].mac[2], peers[i].mac[3], peers[i].mac[4], peers[i].mac[5]); */
/*     } */
/*   } */

/*   void loop() { */
/*   } */
/* }; */
// SWARM STUFF:
//  - When can't connect wifi on fresh boot, periodically
//  * turn on AP, mqtt server, http server storing known good fw & config (for leader)
//  * attempt connect to leader AP / mesh BLE (on ESP32 later) and watch for Homie broadcasts of new wifi details.
//  And/or if plugged serial DMX, RDM route, other 2.4GHz protocol then check that, etc...
// turns out there's already something for it!
//
