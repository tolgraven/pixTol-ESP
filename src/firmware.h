#pragma once

// TODO
#include "log.h"
/* #include "watchdog.h" */

#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

/* #define FW_VERSION "1.0.71" */

class Updater { //holds various OTA update strategies and logic
  public:
    //Updater(const String& id, Strip* s, Blinky* b): _id(id), s(s), b(b) { }
    Updater(const String& id): _id(id) { }
    virtual ~Updater() {}

    //some types use callbacks, some not... hence no pure virtual
    //impl should do whatever specific stuff then call back here for universal
    //which has to involve getting info back so can run animation so fancy
    //
    //FIX: these arent virtual, lambas do simple conversion to our format and calls
    //here, posting an event to device, which informs scheduler, which activates
    //relevant Generator inputter.
    virtual void onStart() {
      /* device->postEvent() */
    }
    virtual void onTick(uint16_t progress, uint16_t total) {
    }
    virtual void onError(const String& error) {}
    virtual void onEnd() {}

    virtual void loop() {
      /* lwd.stamp(PIXTOL_UPDATER); */
    } //feed callback-based updater
    virtual void run() { lg.logf(__func__, Log::WARNING, "From-device update not available for updater %s", _id.c_str()); } //actively attempt update, from our end
  protected:
  String _id;
  /* int prevPixel = -1; */
  //const RgbwColor* otaColor;
  /* const RgbwColor* otaColor = new RgbwColor(70, 70, 200, 50); */
};

class ArduinoOTAUpdater: public Updater { //holds various OTA update strategies and logic
  public:
  ArduinoOTAUpdater(const String& hostname):
    Updater("ArduinoOTA") {

    using namespace std::placeholders;
    ArduinoOTA.onStart(std::bind(&ArduinoOTAUpdater::onStart, this));
    ArduinoOTA.onProgress(std::bind(&ArduinoOTAUpdater::onTick, this, _1, _2));
    ArduinoOTA.onEnd(std::bind(&ArduinoOTAUpdater::onEnd, this));
    ArduinoOTA.onError(std::bind(&ArduinoOTAUpdater::onError, this, _1));

    ArduinoOTA.setHostname(hostname.c_str()); //Homie.getConfiguration().name);
    ArduinoOTA.begin();
  }

  void loop() { ArduinoOTA.handle(); }

  void onStart() { // someone says: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    lg.logf("ArduinoOTA", Log::INFO, "\nOTA flash starting...");
    // b->blink("black");
    //otaColor = ArduinoOTA.getCommand() == U_FLASH?
              //&b->colors["blue"]:
              //&b->colors["yellow"];
  }

  void onTick(uint16_t progress, uint16_t total) {
    // lg.logf("OTA", Log::DEBUG, "%u\t %u", progress, total);
    /* uint16_t pixel   = progress / (total / s->fieldCount()); */
    uint16_t percent = progress / (total / 100);
    /* // ^ progress is fucked and overflowing.... */

    /* if(pixel == prevPixel) { // called multiple times each percent of upload... */
    /*   s->adjustPixel(pixel, "lighten", 20); */
    /*   s->show(); */
    /*   return; */
    /* } */
    /* if(pixel) { // >0 hence -1 >= 0 */
    /*   s->adjustPixel(prevPixel, "darken", 2); */
    /*   for(uint16_t i = pixel; i; i--) { */
    /*     s->adjustPixel(i, "darken", -(i - s->fieldCount())); //tailin */
    /*   } */
    /* } */
    /* s->setPixelColor(pixel, *otaColor); */
    /* s->show(); */
    /* prevPixel = pixel; */
    Serial.printf("OTA updating - %u%%\r", percent);
  }

  void onError(ota_error_t err) {
      // Serial.printf("OTA Error[%u]: ", e);
      String es = (err == OTA_AUTH_ERROR)?    "Auth Failed":
                  (err == OTA_BEGIN_ERROR)?	  "Begin Failed":
                  (err == OTA_CONNECT_ERROR)? "Connect Failed":
                  (err == OTA_RECEIVE_ERROR)? "Receive Failed":
                  (err == OTA_END_ERROR)?		  "End Failed":
                  "Unknown error";
      // LN.log(__func__, Log::ERROR, es);
      /* switch(err) { */
      /*   case OTA_AUTH_ERROR:		lg.log("ArduinoOTA", Log::ERROR, "Auth Failed"); 		break; */
      /*   case OTA_BEGIN_ERROR:		lg.log("ArduinoOTA", Log::ERROR, "Begin Failed");		break; */
      /*   case OTA_CONNECT_ERROR:	lg.log("ArduinoOTA", Log::ERROR, "Connect Failed");	break; */
      /*   case OTA_RECEIVE_ERROR:	lg.log("ArduinoOTA", Log::ERROR, "Receive Failed");	break; */
      /*   case OTA_END_ERROR:			lg.log("ArduinoOTA", Log::ERROR, "End Failed");			break; */
      /* } */
      //b->blink("red", 1);
    }

  void onEnd() {
    lg.log("ArduinoOTA", Log::INFO, "Update successful, rebooting...");
    //b->blink("green", 1);
  }
};


class HomieUpdater: public Updater {
  public:
  HomieUpdater(): Updater("HomieOTA") {}

  void onStart() {
    Serial.println("\nOTA flash starting...");
    lg.logf("OTA", Log::INFO, "Ota ON OTA ON!!!");
    //b->color("white");
  }
  void onTick(uint16_t progress, uint16_t total) { // can use event.sizeDone and event.sizeTotal
    /* uint16_t pixel = progress / (total / s->fieldCount()); */
    /* if(pixel == prevPixel) return; // called multiple times each percent of upload... */
    /* //s->setPixelColor(pixel, b->colors["blue"]); */
    /* Serial.printf("OTA updating - %u%%\r", pixel); */
    /* prevPixel = pixel; */
  }
  void onError(uint8_t error) {
    lg.logf("OTA", Log::WARNING, "OTA FAILE");
    //b->color("red");
  }
  void onEnd() {
    lg.logf("OTA", Log::INFO, "OTA GOODE");
    /* b->blink("green", 5, true); */
  }
};


class HttpUpdater: public Updater {
// class HttpUpdater: public Updater, public HomieNode {
  // http://esp8266.github.io/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#http-server
  // Server respond 200->send fw, 304 n/a. Example header data:
  // [HTTP_USER_AGENT]            => ESP8266-http-Update
  // [HTTP_X_ESP8266_STA_MAC]     => 18:FE:AA:AA:AA:AA
  // [HTTP_X_ESP8266_AP_MAC]      => 1A:FE:AA:AA:AA:AA
  // [HTTP_X_ESP8266_FREE_SPACE]  => 671744
  // [HTTP_X_ESP8266_SKETCH_SIZE] => 373940
  // [HTTP_X_ESP8266_CHIP_SIZE]   => 524288
  // [HTTP_X_ESP8266_SDK_VERSION] => 1.3.0
  // [HTTP_X_ESP8266_VERSION]     => DOOR-7-g14f53a19
  bool flaggedForUpdate = false;
  String _host, _path = "/update/BUILD_ENV_NAME", _currentVersion = "FW_VERSION";
  uint16_t _port;

  void run() {
    // ESPhttpUpdate.updateSpiffs(server.c_str(), currentVersion.c_str()); //etc...
    t_httpUpdate_return r = ESPhttpUpdate.update(_host, _port, _path, _currentVersion);
    switch(r) {
      case HTTP_UPDATE_OK:          onEnd(); break;
      case HTTP_UPDATE_FAILED:      onError(ESPhttpUpdate.getLastError()); break;
      case HTTP_UPDATE_NO_UPDATES:
        lg.logf("httpOTA", Log::INFO, "HTTP OTA no update, ending checks...");
        flagForUpdate(false);
        break;
    }
  }
  public:
  HttpUpdater(const String& host, uint16_t port):
    Updater("HttpOTA"), _host(host), _port(port) {
    ESPhttpUpdate.rebootOnUpdate(false); //ensure time to properly log, run anim?
  }
  void flagForUpdate(bool state = true) { flaggedForUpdate = state; }
  void loop() {
    if(flaggedForUpdate) run();
    // basically, set this flag through MQTT to force check against server - since we don't want to
    // be constantly making requests, or update outside of convenient times.
    // Can still auto update according to some set schedule or when watchdog notices repeat crashes
  }

  void onEnd() {
    lg.logf("httpOTA", Log::INFO, "HTTP OTA ok.");
    // delay(1000);
    ESP.restart();
    // schedule(Time::Seconds, 1, ESP.restart()); //something like this hey
  }
  void onError(uint8_t error) {
    lg.logf("httpOTA", Log::ERROR, "HTTP OTA failed. Error %d: %s\n",
       ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    Updater::onError(ESPhttpUpdate.getLastErrorString().c_str());
  }
};
