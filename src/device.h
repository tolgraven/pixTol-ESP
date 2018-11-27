// firmware, updates, wifi detail swarm sharing stuff...
#pragma once

#include <Homie.h>

#include "config.h"
#include "watchdog.h"
#include "io/unit.h"
#include "io/strip.h"
#include "log.h"
#include "firmware.h"

enum class Status { //based off homie, wrap that first and extend with eg RDM, OPC, HomeKit etc status events
  Invalid = -1, Setup = 1, Normal,
  OtaStart, OtaTick, OtaSuccess, OtaFailure,
  WiFiConnect, WiFiDisconnect,
  MqttConnect, MqttDisconnect, MqttPacket,
  PrepareReset, PrepareSleep
};

class Device { //holds Updater, PhysicalUI/components, more?
  public:
    Device() {}
    bool writeFile();
    void readFile();
    bool writeRTC();
    void readRTC();
    virtual void platformSpecific() {
      lg.logf(__func__, Log::INFO, "Not implemented. Here go esp32/66/whatever specific things.");
      // export $PIOPLATFORM from .ini as PLATFORM_NAME
    }

    std::vector<Updater*> updaters;
    PhysicalUI* ui;
    // WebUI* webUI;

    Status lastStatus;

    void tryWifi() {
      static uint8_t maxAPs = 5;
      bool goStation = false, connected = false;
      wifi_station_ap_number_set(maxAPs); //max number of saved APs
      struct station_config config[maxAPs];
      wifi_station_get_ap_info(config);

      uint8_t lastId = wifi_station_get_current_ap_id(); //which index
      // on connect, then save to spiffs? but hopefully works disconnected, sop can just go down index...
      while(!connected) {
        lastId++;
        if(lastId >= maxAPs) {
          //first check spiffs for further shit
          goStation = true;
          break;
        }
        wifi_station_ap_change(lastId);
        // try connection etc
        // inbetween each, also look for (hardcoded?) mesh leader station, whether it's got news
      }
    }
};

// SWARM STUFF:
//  - When can't connect wifi on fresh boot, periodically turn on AP (for leader) /
//    attempt connect to leader AP / mesh BLE (on ESP32 later) and watch for Homie
//    broadcasts of new wifi details. And/or if plugged old DMX, RDM route, other 2.4GHz protocol
//    check that, etc...
