// firmware, updates, wifi detail swarm sharing stuff...
#pragma once

#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Homie.h>

#include "config.h"
#include "watchdog.h"
#include "io/unit.h"
// #include "state.h"
#include "io/strip.h"

extern Strip* s;
extern Blinky* b;

class Updater { //holds various OTA update strategies and logic
  public:
  Strip* s;
  Blinky* b;
    Updater(Strip* s, Blinky* b): s(s), b(b) {
    }
    virtual ~Updater() {}

    //some types use callbacks, some not... hence no pure virtual
    virtual void onStart() {}
    virtual void onTick(uint16_t progress, uint16_t total) {}
    virtual void onError(uint8_t error) {}
    virtual void onEnd() {}

    virtual bool run() {}
  protected:
    using upCB = std::function<void(const Updater&)>;
    // std::function<void(Updater*)> fStart   = &Updater::onStart;
    // std::function<void(Updater*)> fStart;
    // std::function<void(const Updater&)> fStart;
    // upCB fEnd; // = &Updater::onEnd;
    // std::function<void(const Updater&, uint16_t progress, uint16_t total)> fTick; //= &Updater::onTick;
    // std::function<void(const Updater&, uint8_t error)> fError; //= &Updater::onError;
};

class ArduinoOTAUpdater: public Updater { //holds various OTA update strategies and logic
  int prevPixel = -1;
  const RgbwColor* otaColor;
  public:
  ArduinoOTAUpdater(const String& hostname, Strip* s, Blinky* b): Updater(s, b) {
    using namespace std::placeholders;
    ArduinoOTA.setHostname(hostname.c_str()); //Homie.getConfiguration().name);

    // auto fStart = std::bind(&ArduinoOTAUpdater::onStart, this);
    // ArduinoOTA.onStart(fStart);

    ArduinoOTA.onStart(std::bind(&ArduinoOTAUpdater::onStart, this));
    ArduinoOTA.onProgress(std::bind(&ArduinoOTAUpdater::onTick, this, _1, _1));
    ArduinoOTA.onEnd(std::bind(&ArduinoOTAUpdater::onEnd, this));
    ArduinoOTA.onError(std::bind(&ArduinoOTAUpdater::onError, this, _1));
    // auto fStart = &ArduinoOTAUpdater::onStart;

    // ArduinoOTA.onStart(fStart(*this));
    // ArduinoOTA.onProgress(fTick(*this));
    // std::function<void(int,int)> f = std::bind(&Foo::doSomethingArgs, this, _1, _2);
    //
    // ArduinoOTA.onError(fError(*this));
    // ArduinoOTA.onEnd(fEnd(*this));

    ArduinoOTA.begin();

    if(false) { //fix by watchdog/restartreason
    // if(ESP.getResetReason == REASON_WDT_RST) { //XXX decide which restarts should indicate bootloop and enable this initial few-second throttle
    // extending this (or disabling rel module) first project, for dev smoothness. Auto-http-recovery next stage.
      Homie.getLogger() << "Chance to flash OTA before entering main loop";
      for(int8_t i = 0; i < 10; i++) {
        ArduinoOTA.handle(); // give chance to flash new code in case loop is crashing
        delay(300);
        Homie.getLogger() << ".";
        yield();
      }
      Homie.getLogger() << endl;
    }
  }

  void onStart() {
    Serial.println("\nOTA flash starting...");
    b->blink("black");
    otaColor = ArduinoOTA.getCommand() == U_FLASH?
              // &(b->colors["blue"].Darken(75)): //how make work?
              &b->colors["blue"]:
              &b->colors["yellow"];
  }

  void onTick(uint16_t progress, uint16_t total) {
    uint16_t pixel   = progress / (total / s->fieldCount());
    uint16_t percent = progress / (total / 100);

    if(pixel == prevPixel) { // called multiple times each percent of upload...
      s->adjustPixel(pixel, "lighten", 20);
      s->show();
      return;
    }
    if(pixel) { // >0 hence -1 >= 0
      s->adjustPixel(prevPixel, "darken", 10);
      for(uint16_t i = pixel; i; i--) {
        s->adjustPixel(i, "darken", -(i - s->fieldCount())); //tailin
      }
    }
    s->setPixelColor(pixel, *otaColor);
    s->show();
    prevPixel = pixel;
    Serial.printf("OTA updating - %u%%\r", percent);
  }

// void aOTAOnError(ota_error_t error) {
  void onError(uint8_t error) {
      Serial.printf("OTA Error[%u]: ", error);
      switch(error) {
        case OTA_AUTH_ERROR:		Serial.println("Auth Failed"); 		break;
        case OTA_BEGIN_ERROR:		Serial.println("Begin Failed");		break;
        case OTA_CONNECT_ERROR:	Serial.println("Connect Failed");	break;
        case OTA_RECEIVE_ERROR:	Serial.println("Receive Failed");	break;
        case OTA_END_ERROR:			Serial.println("End Failed");			break;
      }
      b->blink("red", 3);
    }

  void onEnd() { b->blink("green", 3); }
};


class HomieUpdater: public Updater {
  int prevPixel = -1;
  const RgbwColor* otaColor;
  public:
  HomieUpdater(Strip* s, Blinky* b): Updater(s, b) {}

  void onStart() {
    Serial.println("\nOTA flash starting...");
    LN.logf("OTA", LoggerNode::INFO, "Ota ON OTA ON!!!");
    b->color("white");
  }
  void onTick(uint16_t progress, uint16_t total) { // can use event.sizeDone and event.sizeTotal
    uint16_t pixel = progress / (total / s->fieldCount());
    if(pixel == prevPixel) return; // called multiple times each percent of upload...
    s->setPixelColor(pixel, b->colors["blue"]);
    Serial.printf("OTA updating - %u%%\r", pixel);
    prevPixel = pixel;
  }
  // void onError(enum error) {
  void onError(uint8_t error) {
    LN.logf("OTA", LoggerNode::WARNING, "OTA FAILE");
    b->color("red");
  }
  void onEnd() {
    LN.logf("OTA", LoggerNode::INFO, "OTA GOODE");
    b->blink("green", 5, true);
  }
};

const String httpHost = "http://nu.local";
const String httpPath = "/lala/blabla/ota";
class HttpUpdater: public Updater {
  public:
    HttpUpdater(Strip* s, Blinky* b): Updater(s, b) { }

  void onEnd() {
    LN.logf("httpOTA", LoggerNode::INFO, "HTTP OTA ok.");
    delay(1000);
    ESP.restart();
  }
  void onError(uint8_t error) {
    LN.logf("httpOTA", LoggerNode::INFO, "HTTP OTA failed. Error %d: %s\n",
        ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
  }
  // void requestHttpOTA(const String& server = Homie.getConfiguration().otaHost, //get host how?
  void run(const String& server = httpHost,
                      const String& path = httpPath,
                      const String& currentVersion = FW_VERSION) {
  // http://esp8266.github.io/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#http-server
  // Server side script can respond 200 and send the firmware image, 304 if none available.
  // Example header data:
  //  [HTTP_USER_AGENT]            => ESP8266-http-Update
  //  [HTTP_X_ESP8266_STA_MAC]     => 18:FE:AA:AA:AA:AA
  //  [HTTP_X_ESP8266_AP_MAC]      => 1A:FE:AA:AA:AA:AA
  //  [HTTP_X_ESP8266_FREE_SPACE]  => 671744
  //  [HTTP_X_ESP8266_SKETCH_SIZE] => 373940
  //  [HTTP_X_ESP8266_CHIP_SIZE]   => 524288
  //  [HTTP_X_ESP8266_SDK_VERSION] => 1.3.0
  //  [HTTP_X_ESP8266_VERSION]     => DOOR-7-g14f53a19

    ESPhttpUpdate.rebootOnUpdate(false); //ensure time to properly log
    // ESPhttpUpdate.updateSpiffs(server.c_str(), currentVersion.c_str()); //etc...

    t_httpUpdate_return r = ESPhttpUpdate.update(server, 80, path, currentVersion);
    switch(r) {
      case HTTP_UPDATE_OK:          onEnd(); break;
      case HTTP_UPDATE_FAILED:      onError(ESPhttpUpdate.getLastError()); break;
      case HTTP_UPDATE_NO_UPDATES:
        LN.logf("httpOTA", LoggerNode::INFO, "HTTP OTA no update.");
        break;
    }
  }
};


enum class Status { //based off homie, wrap that first and extend with eg RDM, OPC, HomeKit etc status events
  INVALID = -1, SETUP = 1, NORMAL,
  OTA_START, OTA_TICK, OTA_SUCCESS, OTA_FAILURE,
  WIFI_CONNECT, WIFI_DISCONNECT,
  MQTT_CONNECT, MQTT_DISCONNECT, MQTT_PACKET,
  PREPARE_RESET, PREPARE_SLEEP
};

class Device { //holds Updater, PhysicalUI/components, more?
  public:
    Device() {}
    bool writeFile();
    void readFile();
    bool writeRTC();
    void readRTC();

    std::vector<Updater*> updaters;
    PhysicalUI knobheads;

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
