#pragma once

// #include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
// #include <AsyncMqttClient.h>
#if defined(ESP8266)
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <mDNS.h>
#endif
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <smooth/core/Task.h>
// #include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// #include <cont.h> //somethong about continuations, might come handy
/* #include <WifiEspNowBroadcast.h> */
/* #include <esp_now.h> */

#include "base.h"
#include "log.h"

namespace tol {

struct MDNSService {
  std::string name, protocol; //or just enum lol
  uint16_t port;
  struct InfoField { std::string name, value; };
  std::vector<InfoField> infoFields;
};

class NetworkConnection: public Named, public smooth::core::Task { // wifi + mesh/Now, bt, anything else ad-hoc? or generic enough for MQTT etc? tho diff cat
  public:
  NetworkConnection(const std::string& id, const std::string& type):
    Named(id, type), smooth::core::Task(("NetworkConnection " + id).c_str(), 3072, 8, std::chrono::seconds(10)) {}
  virtual ~NetworkConnection() {}

  virtual bool setup() = 0;
  virtual void stop() = 0;
  virtual bool isConnected() = 0;
  virtual bool resetConfiguration() = 0;
  virtual void onConfigMode() {}
  virtual void onConfigured() {}
  virtual void onConnected() {}
  virtual void onDisconnected() {}
};
// afa wrapping wifimanager think support for pre-set creds and auto or no fallback, manual cfg reset and conf AP...
class WifiConnection: public NetworkConnection {
  std::unique_ptr<WiFiManager> wifiManager;
  std::vector<WiFiManagerParameter> customParams; // After connecting, parameter.getValue() will get you the configured value id/name placeholder/prompt default length

  std::string hotspotDefaultPassword = "baconmanna";

  struct IfConfig { // blabla anyways need a central point for easy reconfiguration
    IPAddress ip, subnetMask, gateway;
    bool dhcp;
  };
  public:
  WifiConnection(const std::string& id, const std::string& type):
   NetworkConnection("wifi", type) { //provide some kinda (likely subset of) config obj...

    wifiManager = std::make_unique<WiFiManager>(); //unlikely this obj itself will be needed for so long.
    // not sure how best think about it. maybe delete end of start() but might want be able use without rebooting too

#ifndef DEBUG
    // wifiManager->setDebugOutput(false); //silence
#endif
    // wifiManager->setSaveConfigCallback(saveConfigCallback); //can use std::bind etc? check
    // wifiManager->setSTAStaticIPConfig(ips[NET], ips[GATEWAY], ips[MASK]); // need basic common conf for this typa...
    // wifiManager->setAPStaticIPConfig(); // for portal...
    wifiManager->setRemoveDuplicateAPs(false);
    wifiManager->setMinimumSignalQuality(0); //set minimu quality of signal so it ignores AP's under that quality defaults to 8%
    wifiManager->setTimeout(300); //timeout til conf portal turns off, useful to make it all retry or go to sleep in seconds
    using namespace std::placeholders;
    wifiManager->setAPCallback(std::bind(&WifiConnection::onConfigMode, this)); // when fails to connect and launches hotspot
    // 
    // inject custom head element You can use this to any html bit to the head of the configuration portal. If you add a <style> element, bare in mind it overwrites the included css, not replaces.
    wifiManager->setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
    // inject a custom bit of html in the configuration form
    WiFiManagerParameter custom_text("<p>pixTol in the houze</p>");
    wifiManager->addParameter(&custom_text);

    // customParams.emplace_back("server", "mqtt server", mqtt_server, 40); // eh these p clunky lol
    // customParams.emplace_back("universe", "dmx", mqtt_port, 6);
    // for(auto&& param: customParams) wifiManager->addParameter(&param);
    //strcpy(mqtt_server, custom_mqtt_server.getValue());
    //strcpy(mqtt_port, custom_mqtt_port.getValue());
    // saveConfig(shouldSaveConfig); //read/save updated parameters
    // wifiManager->setHostname(id().c_str());
    // wifiManager->setHostname(id.c_str()); // uh causes garbage, weird...
  }
  virtual ~WifiConnection() { if(isConnected()) stop(); } // could nuke earlier, as soon done setting basic settings (then rest should be mqtt, RDM, own page or whatever)

  WiFiManager* getManager() { return wifiManager.get(); }

  void init() override { setup(); }

  bool setup() override {
   // IDEA WAS:
   // first fire an event, start a timer periodically pushing msg we're in connec mode...
   // but since we're in their context can't actually properly act on messages anyways
   // so fork lib and can add poll-to-run also during connect,
   // not actually deferring boot til connect, multiAP, etc...

    // wifiManager->setHostname(id().c_str());
    // auto eventSender = fn;
    if(!wifiManager->autoConnect(id().c_str(), hotspotDefaultPassword.c_str())) {
      lg.err("Failed to connect to Wifi or start configuration hotspot..", __func__);
      wifiManager.reset(nullptr);
      return false; // our caller will have to ensure light starts flashing red and shit asplode
    }
#ifdef ESP8266
    std::string chipId = std::string(ESP.getChipId(), HEX);
    MDNS.setInstanceName((id() + " (" + chipId + ")").c_str()); // printf("%s (%s)", );

    if(MDNS.begin(id().c_str())) {
      MDNSService services[2] = {{"mqtt",    "tcp", 1883},
                                 {"art-net", "udp", 6564, {{"Manufacturer", "tolgrAVen"}}}}; // no go yet needs moar auto

       for(auto svc: services) { //however not hapnening here. depends on ports/services in use. Prob they tasked sending to here.
        MDNS.addService(svc.name, svc.protocol, svc.port);
        for(auto& txt: svc.infoFields)
        MDNS.addServiceTxt(svc.name, svc.protocol, txt.name, txt.value);
       } // MDNS.addService("e131", "udp", E131_DEFAULT_PORT); MDNS.addService("http", "tcp", HTTP_PORT);
       // MDNS.addServiceTxt("e131", "udp", "TxtVers", std::string(RDMNET_DNSSD_TXTVERS)); MDNS.addServiceTxt("e131", "udp", "ConfScope", RDMNET_DEFAULT_SCOPE); MDNS.addServiceTxt("e131", "udp", "E133Vers", std::string(RDMNET_DNSSD_E133VERS));
      }
#endif
      wifiManager.reset(nullptr);
      return true;
    }
    virtual void stop() {
     WiFi.disconnect(); // disconnect/sleep wifi. use esp fns directly
    }
    virtual bool resetConfiguration() {
      if(!wifiManager.get())
        wifiManager = std::make_unique<WiFiManager>();
     wifiManager->resetSettings(); //reset settings - for testing
     wifiManager->startConfigPortal(); // like autoConnect but manually...
     return true; // I guess.
    }
    virtual bool isConnected() { return true; }
    virtual void onConfigMode() { DEBUG("WIFI AP PSAWNED"); }
  };
  // class BTConnection: public NetworkConnection {
  //   BTConnection(): NetworkConnection() { }
  //   virtual ~BTConnection() {}
  // }; // and then some fancier overlays for swarm stuff ESPnow
  // also semi related to inputter/outputter concept and how they can be semi medium agnostic
  // so ex in some cases might want to use different kinds of 2.4Ghz or whatever


  //void tryWifi() {
  //  static uint8_t maxAPs = 5;
  //  /* bool goStation = false; */
  //  bool connected = false;
  //  wifi_station_ap_number_set(maxAPs); //max number of saved APs
  //  struct station_config config[maxAPs];
  //  wifi_station_get_ap_info(config);

  //  uint8_t lastId = wifi_station_get_current_ap_id(); //which index
  //  // on connect, then save to spiffs? but hopefully works disconnected, sop can just go down index...
  //  while(!connected) {
  //    lastId++;
  //    if(lastId >= maxAPs) {
  //      //first check spiffs for further shit
  //      /* goStation = true; */
  //      break;
  //    }
  //    wifi_station_ap_change(lastId);
  //    // try connection etc
  //    // inbetween each, also look for (hardcoded?) mesh leader station, whether it's got news
  //  }
  //}

// #define ESP_NOW_MAX_PEERS 20

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

}
