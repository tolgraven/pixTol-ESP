#pragma once

#include <functional>
#include <map>
/* #include <Homie.h> */

/* #include "battery.h" */
#include "config.h"
#include "log.h"
#include "debuglog.h"
#include "device.h"
#include "firmware.h"


class IOT { //XXX better name/plan. merge with Device?
  String _id;
  /* BatteryNode* battery; */
  /* const HomieEvent* lastEvent = nullptr; */
  uint8_t disconnects = 0;
  uint32_t wasOnlineAt;
  bool firstConnect = true;
  Updater* homieOTAUpdater;

  public:
  /* ConfigNode* cfg; */
  Device* device;
  /* std::map<String, HomieNode*> node; */

  // using PropertyCallback = std::function<bool(const HomieRange&, const String&)>;
  // using NodeCallback = std::function<bool(const String&, const HomieRange&, const String&)>;
  // struct PropertyConfig {
  //   String id;
  //   bool settable = true;
  //   int range[2] = {0, 0}; // = no range
  //   // std::pair<int, int> range;
  //   PropertyCallback handler = nullptr;
  // }; //std::function<void(const String& value)>
  //
  // struct NodeConfig {
  //   String id, type;
  //   NodeCallback handler = nullptr;
  //   std::vector<PropertyConfig> properties;
  //   NodeConfig(const String& id, const String& type, std::vector<PropertyConfig> properties, NodeCallback handler = nullptr):
  //     id(id), type(type), properties(properties), handler(handler) {
  //   }
  // };
  // PropertyCallback& getPropCB() {}

  uint8_t blendMode = 0;

  /* IOT(const String& id, Device* device): _id(id), cfg(new ConfigNode()), device(device) { */
  IOT(const String& id, Device* device): _id(id), device(device) {
    /* homieOTAUpdater = new HomieUpdater(); */
    using namespace std::placeholders;

    // for(auto n: nodeConfigurations) {
    //   node[n.id] = new HomieNode(n.id, n.type);
    //   for(auto p: n.properties) {
    //     HomieThingreturnwhatis& thing;
    //     if(p.range[1]) //is range
    //       n->advertiseRange(p.id, p.range[0], p.range[1]);
    //     else
    //       n->advertise(p.id);
    //
    //     if(p.handler)
    //       thing.settable(handler);
    //     else if(n.handler && p.settable)
    //       thing.settable();
    //   }
    // }
    // YO: thing is tho, all cmds should just get thrown to common parser, so can handle serial, web, whatever input...
    /* node["mode"] = new HomieNode("mode", "mode"); // mode = new HomieNode("mode", "mode"); */
    /* /1* node["mode"]->advertiseRange("controls", 1, 12).settable(std::bind(&IOT::controlsHandler, this, _1, _2)); *1/ */
    /* /1* node["mode"]->advertiseRange("blend", 0, 12).settable(std::bind(&IOT::blendHandler, this, _1, _2)); *1/ */
    /* node["mode"]->advertise("power").settable(std::bind(&IOT::powerHandler, this, _1, _2)); //just on/off for Homekit etc... */

    /* node["status"] = new HomieNode("status", "log"); */
    /* const String statusProperties[] = */
    /* {"freeHeap", "heapFragmentation", "maxFreeBlockSize", */
    /*   "fps", "droppedFrames", */
    /*  "dimmer.base", "dimmer.force", "dimmer.out", "dimmer.strip"}; */
    /* for(auto& prop: statusProperties) node["status"]->advertise(prop.c_str()); */

    /* node["output"] = new HomieNode("outputs", "io", std::bind(&IOT::outputHandler, this, _1, _2, _3)); */
    /* node["output"]->advertise("strip").settable(); */
    /* node["input"] = new HomieNode("inputs", "io", std::bind(&IOT::inputHandler, this, _1, _2, _3)); */
    /* node["input"]->advertise("artnet").settable(); */

    /* // below example of challenge. for config mix props and ranges, maybe pull from spiffs like */
    /* // color */
    /* // rgbw 1 4 */
    /* // rgb 1 3 */
    /* // std::map<String, std::pair*>, then attempt make pair of anything after first word? not null then advertiseRange... */
    /* // then ideally callbacks formed mostly dynamically, from pre-set blocks, */
    /* // "save to dest"(var, cfg... likely existing with same id in whatever lookup map) */
    /* // "call fn"(wrap needed since cant skip homie cb step) */
    /* // "emit event" / "signal" */
    /* // all kinda same... basically gotta figure out how to auto assoc stuff */
    /* // and then have em handled nicely through globalInputHandler */
    /* node["color"] = new HomieNode("color", "fx", std::bind(&IOT::colorHandler, this, _1, _2, _3)); */
    /* node["color"]->advertise("color").settable(); */
    /* node["color"]->advertiseRange("rgbw", 1, 4).settable(); */
    /* node["color"]->advertiseRange("rgb", 1, 3).settable(); */
    /* node["color"]->advertiseRange("hsl", 1, 3).settable(); */
    /* node["color"]->advertise("cmd").settable(); */

    /* Homie_setFirmware("FW_NAME", "FW_VERSION"); */
    /* Homie_setBrand("FW_BRAND"); */
    /* Homie.setConfigurationApPassword("FW_NAME"); //not working right? */

    /* // Homie.setLedPin(D2, HIGH); Homie.disableLedFeedback(); */

    /* Homie.onEvent(std::bind(&IOT::onHomieEvent, this, _1)); */
    /* Homie.setGlobalInputHandler(std::bind(&IOT::globalInputHandler, this, _1, _2, _3, _4)); */
    /* /1* Homie.setBroadcastHandler(std::bind(&IOT::broadcastHandler, this, _1, _2)); *1/ */
    /* // Homie.setSetupFunction(homieSetup) */
    /* // Homie.setLoopFunction(homieLoop); */

    /* /1* if(analogRead(0) > 100) battery = new BatteryNode(); // hella unsafe, should be a setting *1/ */
    /* // XXX plus gotta normalize since ESP32 12bit not 10? */
    /* // if(!cfg->debug.get()) Homie.disableLogging(); */

    /* /1* lg.initOutput("Homie"); //needs to be done before setup i think *1/ */

    /* Homie.setup(); */

    /* finishSetup(); //get rid of now tho... */
    /* lg.dbg("Done Homie and nodes"); */
  }

  ~IOT() {
    /* delete debug; */
  }

  void finishSetup() {
    /* lg.setStatusNode(node["status"]); */
  }

  void loop() {
    /* lg.dbg("IOT loop"); */
    /* Homie.loop(); */

    /* static bool connectedLastLoop = true; */
    static bool connectedLastLoop = false;
    /* if(Homie.isConnected()) { // kill any existing go-into-wifi-finder timer, etc */
    /*   if(firstConnect) { */
    /*     // could do this in HomieEvent wifi thing I guess tho */
    /*     // device->debug->bootLog(Debug::doneONLINE); //these break linker atm :/ */
    /*     // device->debug->bootInfoPerMqtt(); */
    /*     firstConnect = false; */
    /*   } */
    /*   wasOnlineAt = micros(); */
    /*   connectedLastLoop = true; */
    /* } else { // stays stuck in this state for a while on boot... set a timer if not already exists, yada yada blink statusled for sure... */
    /*   if(!firstConnect) { //already been connected, so likely temp hickup, chill */
    /*     if(micros() - wasOnlineAt < 10 * MILLION) //give it 10s */
    /*     // if(Time::passed(10, wasOnlineAt, Seconds)) //want something like this? */
    /*       return; */
    /*     else { */
    /*       if(connectedLastLoop) { */
    /*         lg.f("Wifi", Log::DEBUG, "Down for over 10 seconds... sup?"); */
    /*         connectedLastLoop = false; */
    /*       } */
    /*     } */
    /*   } else { // fade up some sorta boot anim I guess? */
    /*   } */
    /* } */
  }

  /* bool outputHandler(const String& property, const HomieRange& range, const String& value) { */
  /*   /1* node["output"]->setProperty(property).send(value); //i mean for sure automate this shit.. *1/ */

  /*   if(property == "strip") { // XXX well it should disable the object not fuck w pinmode lol */
  /*     if(value == "on") pinMode(RX, OUTPUT); //no effect... */
  /*     else              pinMode(RX, INPUT_PULLUP); //works */
  /*   } else if(property == "serial") { */
  /*   } else if(property == "debug") { //toggle using strip for diagnostics (later obv statusleds instead anyways) */
  /*   } */
  /*   return true; */
  /* } */

  /* bool inputHandler(const String& property, const HomieRange& range, const String& value) { */
  /*   /1* node["input"]->setProperty(property).send(value); *1/ */

  /*   bool on = (value == "on"); */
  /*   // future: for registered inputters; if property==inputter.name; pass the message... */
  /*   if(property == "artnet") { */
  /*   } else if(property == "mqtt") { */
  /*     if(on) { // set so color received goes out (should be default tho) */
  /*     } else { } */
  /*   } else if(property == "serial") { */
  /*     if(on) { //configure serial port for data input */
  /*     } else { } */
  /*   } else if(property == "dmx") { */
  /*     if(on) { } else { } */
  /*   } */
  /*   return true; */
  /* } */

  /* /1* bool controlsHandler(const HomieRange& range, const String& value) { *1/ */
  /* /1*   if(range.index <= f->numChannels) { *1/ */
  /* /1*     f->chOverride[range.index-1] = value.toInt(); *1/ */
  /* /1*     node["mode"]->setProperty("controls").setRange(range).send(value); *1/ */
  /* /1*     return true; *1/ */
  /* /1*   } return false; *1/ */
  /* /1* } *1/ */
  /* /1* bool blendHandler(const HomieRange& range, const String& value) { *1/ */
  /* /1*   if(range.index == 0) { //piggyback pixel blend mode ctrl here *1/ */
  /* /1*     blendMode = value.toInt(); *1/ */
  /* /1*   } else if(range.index <= f->numChannels) { *1/ */
  /* /1*     f->blendOverride[range.index-1] = value.toFloat(); *1/ */
  /* /1*     // node["mode"]->setProperty("blend").setRange(range).send(value); *1/ */
  /* /1*   } else return false; *1/ */
  /* /1*   node["mode"]->setProperty("blend").setRange(range).send(value); *1/ */
  /* /1*   return true; *1/ */
  /* /1* } *1/ */

  /* bool powerHandler(const HomieRange& range, const String& value) { */
  /*   if(value == "fullreset")    { Homie.reset(); //total reset, stay safe yo */
  /*   } else if(value == "restart") { */
  /*     ESP.restart(); //rather go through device so can set lwd etc */
  /*     return true; */
  /*     // device->restart(); */
  /*   } else if(value == "ledoff") { */
  /*     Homie.disableLedFeedback(); */
  /*     return true; */
  /*   } */
  /*   return false; */
  /* } */

  /* RgbwColor globalRgbwColor = RgbwColor(255, 30, 20, 20); */
  /* HslColor globalHslColor = HslColor(0.1, 0.42, 0.56); */
  /* RgbwColor* globalOutputColor = &globalRgbwColor; //should obv be one and the same later updating both with each... */

  /* bool colorHandler(const String& property, const HomieRange& range, const String& value) { */
  /*   bool wasSuccessful = false; */

  /*   /1* if(property == "color") { *1/ */
  /*     /1* if(b->color(value)) { //set succeeded (color found in palette, qnd applied) *1/ */
  /*     /1*   globalRgbwColor = b->colors[value]; *1/ */
  /*     /1*   wasSuccessful = true; *1/ */
  /*     /1* } *1/ */
  /*   /1* } else if(property == "cmd") { *1/ */
  /*     /1* if(value == "test") b->test(); //will then return false but eh *1/ */
  /*   /1* } else if(property == "rgbw") { *1/ */
  /*   if(property == "rgbw") { */
  /*     wasSuccessful = true; */
  /*     switch(range.index) { */
  /*       case 1: globalRgbwColor.R = value.toInt(); break; */
  /*       case 2: globalRgbwColor.G = value.toInt(); break; */
  /*       case 3: globalRgbwColor.B = value.toInt(); break; */
  /*       case 4: globalRgbwColor.W = value.toInt(); break; */
  /*       default: wasSuccessful = false; break; */
  /*     } */
  /*     globalOutputColor = &globalRgbwColor; */
  /*   } else if(property == "hsl") { */
  /*     wasSuccessful = true; */
  /*     switch(range.index) { */
  /*       case 1: globalHslColor.H = value.toFloat(); break; */
  /*       case 2: globalHslColor.S = value.toFloat(); break; */
  /*       case 3: globalHslColor.L = value.toFloat(); break; */
  /*       default: wasSuccessful = false; break; */
  /*     } */
  /*     globalOutputColor = new RgbwColor(globalHslColor); */
  /*     globalOutputColor->W = 50; //w/e, test, get both proper conversion and global white ctrl in place */
  /*   } */

  /*   if(wasSuccessful) { */
  /*     /1* s->setColor(*globalOutputColor); *1/ */
  /*   } */
  /*   lg.dbg("Set color went:" + (String)wasSuccessful); */
  /*   return wasSuccessful; */
  /* } */

  /* // bool settingsHandler(const HomieRange& range, const String& value) { */
  /* //   mode.setProperty("settings").send(value); */
  /* //   return true; // was thinking interframes, strobehz, dmxhz, log, flipped. but eh so much wooorke */
  /* // } */

  /* bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value) { */
  /*   // static std::map<String, std::function<void(const String& value)>> actions; //this'd be koool */
  /*   if(property == "mode") { */
  /*     if(value == "dmx") { */
  /*     } else if(value == "mqtt") { */
  /*     } else if(value == "standalone") { */
  /*     } */
  /*   } else return false; //leave to other handlers */
  /*   return true; */
  /* } */

  /* bool broadcastHandler(const String& level, const String& value) { // not sure what to use for but will prob get something... */
  /*   lg.f("BROADCAST", Log::INFO, "Level %s: %s", level.c_str(), value.c_str()); */
  /*   return true; */
  /* } */

  /* void onHomieEvent(const HomieEvent& event) { // REMEMBER: It is not possible to execute delay() or yield() from an asynchronous callback. */
  /*   /1* lg.dbg("Received HomieEvent "); *1/ */
  /*   lg.f(__func__, Log::DEBUG, "Received HomieEvent %d\n", event.type); */
  /*   /1* static Blinky* pre; *1/ */
  /*   /1* static int prevPixel = -1; *1/ */
  /*   /1* lastEvent = &event; //can do this? or fall outta scope badd? so wanna copy... *1/ */
  /*   switch(event.type) { */
  /*     case HomieEventType::STANDALONE_MODE: break; */
  /*     case HomieEventType::CONFIGURATION_MODE: */
  /*       // likely some shitty error from malformed config, attempt to restore */
  /*       // keep config file backups on spiffs, check for last valid one */
  /*       // and/or ask swarm, and/or etc */
  /*       /1* pre = new Blinky(RGBW, 300); *1/ */
  /*       /1* pre->gradient("yellow", "orange"); *1/ */
  /*       /1* delete pre; *1/ */
  /*       break; */
  /*     case HomieEventType::NORMAL_MODE: */
  /*       /1* pre = new Blinky(RGBW, 125); *1/ */
  /*       /1* pre->gradient("yellow", "green"); *1/ */
  /*       /1* delete pre; *1/ */
  /*       break; */

  /*     /1* case HomieEventType::OTA_STARTED: *1/ */
  /*     /1*   homieOTAUpdater->onStart(); break; *1/ */
  /*     /1* case HomieEventType::OTA_PROGRESS: *1/ */
  /*     /1*   homieOTAUpdater->onTick(event.sizeDone, event.sizeTotal); break; *1/ */
  /*     /1* case HomieEventType::OTA_FAILED: *1/ */
  /*     /1*   homieOTAUpdater->onError(0); break; *1/ */
  /*     /1* case HomieEventType::OTA_SUCCESSFUL: *1/ */
  /*     /1*   homieOTAUpdater->onEnd(); break; *1/ */

  /*     case HomieEventType::ABOUT_TO_RESET: */
  /*       Homie.setIdle(false); */
  /*       /1* b->gradient("red", "yellow"); *1/ */
  /*       lg.f("Device", Log::WARNING, "Going down for reset...\n"); */
  /*       Homie.setIdle(true); */
  /*       break; */

  /*     case HomieEventType::WIFI_CONNECTED: */
  /*       /1* b->gradient("orange", "green"); *1/ */
  /*       break; // normal mode, can use event.ip, event.gateway, event.mask */
  /*     case HomieEventType::WIFI_DISCONNECTED: */
  /*       disconnects++; */
  /*       /1* b->gradient("orange", "red"); //XXX will be: start animation with base *1/ */
  /*       lg.f("Wifi", Log::WARNING, "disconnected, reason %u\n", event.wifiReason); */
  /*       /1* switch(event.wifiReason) { //then: adapt animation target from spec reason... *1/ */
  /*       /1*   case WIFI_DISCONNECT_REASON_NO_AP_FOUND: *1/ */
  /*       /1*     lg.f("Wifi", Log::WARNING, "No AP found...\n"); break; *1/ */
  /*       /1* } *1/ */
  /*       //XXX 4 was happening to esps left on long after net briefly unavail, then came back. */
  /*       //    clear conn/auto reboot if stuck on same reason for too long? a la updater autorecovery. */
  /*       //    also maybe periodically check: */
  /*       //    -ping to router, main server, other units */
  /*       //    adapt interpolation/keyFrameRate and prep/activate compensatory measures when detected */
  /*       // */
  /*       // WIFI_DISCONNECT_REASON_UNSPECIFIED              = 1, */
  /*       // WIFI_DISCONNECT_REASON_AUTH_EXPIRE              = 2, */
  /*       // WIFI_DISCONNECT_REASON_AUTH_LEAVE               = 3, */
  /*       // WIFI_DISCONNECT_REASON_ASSOC_EXPIRE             = 4, */
  /*       // WIFI_DISCONNECT_REASON_ASSOC_TOOMANY            = 5, */
  /*       // WIFI_DISCONNECT_REASON_NOT_AUTHED               = 6, */
  /*       // WIFI_DISCONNECT_REASON_NOT_ASSOCED              = 7, */
  /*       // WIFI_DISCONNECT_REASON_ASSOC_LEAVE              = 8, */
  /*       // WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED         = 9, */
  /*       // WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD      = 10,  /1* 11h *1/ */
  /*       // WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD     = 11,  /1* 11h *1/ */
  /*       // WIFI_DISCONNECT_REASON_IE_INVALID               = 13,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_MIC_FAILURE              = 14,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS       = 17,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID     = 18,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID  = 19,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_AKMP_INVALID             = 20,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION    = 21,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP       = 22,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED       = 23,  /1* 11i *1/ */
  /*       // WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED    = 24,  /1* 11i *1/ */
  /*       // */
  /*       // WIFI_DISCONNECT_REASON_BEACON_TIMEOUT           = 200, */
  /*       // WIFI_DISCONNECT_REASON_NO_AP_FOUND              = 201, */
  /*       // WIFI_DISCONNECT_REASON_AUTH_FAIL                = 202, */
  /*       // WIFI_DISCONNECT_REASON_ASSOC_FAIL               = 203, */
  /*       // WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT        = 204, */
  /*       break; */
  /*     case HomieEventType::MQTT_READY: */
  /*       // if(mqttFirstConnect) { */
  /*       /1* b->gradient("blue", "green"); *1/ */
  /*         // mqttFirstConnect = false; */
  /*       // } */
  /*       break; */
  /*     case HomieEventType::MQTT_DISCONNECTED: */
  /*       /1* b->gradient("blue", "red"); *1/ */
  /*       break; // can use event.mqttReason */

  /*     case HomieEventType::MQTT_PACKET_ACKNOWLEDGED: break; // MQTT packet with QoS > 0 is acknowledged by the broker, can use event.packetId */
  /*     case HomieEventType::READY_TO_SLEEP: break; // After calling prepareToSleep() the event is triggered when MQTT is disconnected */
  /*     /1* case HomieEventType::SENDING_STATISTICS: break; // something warned about not handling this, then that it's not there. ugh *1/ */
  /*   } */
  /* } */
};
