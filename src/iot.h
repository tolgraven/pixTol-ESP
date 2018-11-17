#pragma once

#include <functional>
#include <Homie.h>
#include <LoggerNode.h>

#include "io/strip.h"
#include "battery.h"
#include "config.h"
#include "debug.h"
#include "device.h"

class IOT { //XXX better name/plan. merge with Device?
  String _id;
  Strip* s;
  Functions* f;
  Blinky* b;
  BatteryNode* battery;
  const HomieEvent* lastEvent = nullptr;
  uint8_t disconnects = 0;
  uint16_t wasOnlineAt;
  bool firstConnect = true;

  std::map<String, Updater*> updater;

  public:
  Debug* debug;
  ConfigNode* cfg;
  HomieNode *mode, *status, *output, *input, *color;
  IOT(const String& id): _id(id), cfg(new ConfigNode()) {
    using namespace std::placeholders;
    mode = new HomieNode("mode", "mode");
    status = new HomieNode("status", "log");
    output = new HomieNode("outputs", "io", std::bind(&IOT::outputHandler, this, _1, _2, _3));
    input = new HomieNode("inputs", "io", std::bind(&IOT::inputHandler, this, _1, _2, _3));
    color = new HomieNode("color", "fx", std::bind(&IOT::colorHandler, this, _1, _2, _3));

    Homie_setFirmware(FW_NAME, FW_VERSION);
    Homie_setBrand(FW_BRAND);
    Homie.setConfigurationApPassword(FW_NAME);

    // Homie.setLedPin(D2, HIGH); Homie.disableLedFeedback();
    Homie.onEvent(std::bind(&IOT::onHomieEvent, this, _1));
    Homie.setGlobalInputHandler(std::bind(&IOT::globalInputHandler, this, _1, _2, _3, _4));
    Homie.setBroadcastHandler(std::bind(&IOT::broadcastHandler, this, _1, _2));
    // Homie.setSetupFunction(homieSetup).setLoopFunction(homieLoop);

    if(analogRead(0) > 100) battery = new BatteryNode(); // hella unsafe, should be a setting
    // if(!cfg->debug.get()) Homie.disableLogging();

    mode->advertiseRange("controls", 1, 12).settable(std::bind(&IOT::controlsHandler, this, _1, _2));
    mode->advertiseRange("blend", 1, 12).settable(std::bind(&IOT::blendHandler, this, _1, _2));
    mode->advertise("power").settable(std::bind(&IOT::powerHandler, this, _1, _2)); //just on/off for Homekit etc...

    color->advertise("color").settable();
    color->advertiseRange("rgbw", 1, 4).settable();
    color->advertiseRange("rgb", 1, 3).settable();
    color->advertiseRange("hsl", 1, 3).settable();
    color->advertise("cmd").settable();

    output->advertise("strip").settable();
    input->advertise("artnet").settable();

    const String statusProperties[] = {"freeHeap", "heapFragmentation", "maxFreeBlockSize",
                                "vcc", "fps", "droppedFrames",
                                "dimmer.base", "dimmer.force", "dimmer.out"};
    for(String prop: statusProperties) status->advertise(prop.c_str());

    Homie.setup();
  }
  ~IOT() {
    updater.clear();
    delete debug;
  }

  void finishSetup(Strip* strip, Blinky* blinky, Functions* functions) {
    s = strip;
    b = blinky;
    f = functions;
    updater["OTA"] = new ArduinoOTAUpdater(_id, s, b);
    updater["HomieOTA"] = new HomieUpdater(s, b);
    debug = new Debug(status, s, f, cfg);
  }

  void loop() {
    updater["OTA"]->loop();
    Homie.loop();

    if(Homie.isConnected()) { // kill any existing go-into-wifi-finder timer, etc
      if(firstConnect) {
        // debug->bootLog(Debug::doneONLINE); //these break linker atm :/
        // debug->bootInfoPerMqtt();
        firstConnect = false;
      }
      wasOnlineAt = millis();
    } else { // stays stuck in this state for a while on boot... set a timer if not already exists, yada yada blink statusled for sure...
      if(!firstConnect) { //already been connected, so likely temp hickup, chill
        if(millis() - wasOnlineAt < 10000) //give it 10s
          return;
        else {
          static bool justDisconnected = true;
          if(justDisconnected) {
            LN.logf(_DEBUG_, "Wifi appears well down");
            justDisconnected = false;
          }
        }
      } else { // fade up some sorta boot anim I guess?
      }
    }
  }

  bool outputHandler(const String& property, const HomieRange& range, const String& value) {
    output->setProperty(property).send(value);
    LN.logf(__func__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

    if(property == "strip") {
      if(value == "on") pinMode(RX, OUTPUT); //no effect...
      else              pinMode(RX, INPUT_PULLUP); //works
    } else if(property == "serial") {
    } else if(property == "debug") { //toggle using strip for diagnostics (later obv statusleds instead anyways)
    }
    return true;
  }

  bool inputHandler(const String& property, const HomieRange& range, const String& value) {
    input->setProperty(property).send(value);
    LN.logf(__func__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

    bool on = (value == "on");
    // future: for registered inputters; if property==inputter.name; pass the message...
    if(property == "artnet") {
    } else if(property == "mqtt") {
      if(on) { // set so color received goes out (should be default tho)
      } else { }
    } else if(property == "serial") {
      if(on) { //configure serial port for data input
      } else { }
    } else if(property == "dmx") {
      if(on) { } else { }
    }
    return true;
  }

  bool controlsHandler(const HomieRange& range, const String& value) {
    if(range.index <= f->numChannels) {
      f->chOverride[range.index-1] = value.toInt();
      mode->setProperty("controls").setRange(range).send(value);
      return true;
    }
    return false;
  }
  bool blendHandler(const HomieRange& range, const String& value) {
    if(range.index <= f->numChannels) {
      f->blendOverride[range.index-1] = value.toFloat();
      mode->setProperty("blend").setRange(range).send(value);
      return true;
    }
    return false;
  }

  bool powerHandler(const HomieRange& range, const String& value) {
    //yada
  }

  RgbwColor globalRgbwColor = RgbwColor(255, 30, 20, 20);
  HslColor globalHslColor = HslColor(0.1, 0.42, 0.56);
  RgbwColor* globalOutputColor = &globalRgbwColor; //should obv be one and the same later updating both with each...

  bool colorHandler(const String& property, const HomieRange& range, const String& value) {
    LN.logf(__func__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());
    bool wasSuccessful = false;

    if(property == "color") {
      if(b->color(value)) { //set succeeded (color found in palette)
        globalRgbwColor = b->colors[value];
        wasSuccessful = true;
      }
    } else if(property == "cmd") {
      if(value == "test") b->test();
    } else if(property == "rgbw") {
      wasSuccessful = true;
      switch(range.index) {
        case 1: globalRgbwColor.R = value.toInt(); break;
        case 2: globalRgbwColor.G = value.toInt(); break;
        case 3: globalRgbwColor.B = value.toInt(); break;
        case 4: globalRgbwColor.W = value.toInt(); break;
        default: wasSuccessful = false; break;
      }
      globalOutputColor = &globalRgbwColor;
    } else if(property == "hsl") {
      wasSuccessful = true;
      switch(range.index) {
        case 1: globalHslColor.H = value.toFloat(); break;
        case 2: globalHslColor.S = value.toFloat(); break;
        case 3: globalHslColor.L = value.toFloat(); break;
        default: wasSuccessful = false; break;
      }
      globalOutputColor = new RgbwColor(globalHslColor);
      globalOutputColor->W = 70; //w/e, test, get proper conversion later...
    }
    if(wasSuccessful) {
      s->setColor(*globalOutputColor);
    }
    return wasSuccessful;
  }

  // bool settingsHandler(const HomieRange& range, const String& value) {
  //   mode.setProperty("settings").send(value);
  //   return true; // was thinking interframes, strobehz, dmxhz, log, flipped. but eh so much wooorke
  // }

  bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value) {
    LN.logf(node.getId(), LoggerNode::INFO, "%s: %s", property.c_str(), value.c_str());

    // static std::map<String, std::function<void(const String& value)>> actions; //this'd be koool
    if(property == "command") {
      if(value == "reset")        Homie.reset(); //total reset, stay safe yo
      if(value == "restart")      ESP.restart();
      else if(value == "ledoff")  Homie.disableLedFeedback();
    } else if(property == "mode") {
      if(value == "dmx") {
        // eventually multiple input/output types etc, LTP/HTP/average/prio, specific mergin like
        // animation over serial, DMX FN CH from artnet
        // animation over artnet, fn ch overrides from mqtt
      } else if(value == "mqtt") {
      } else if(value == "standalone") {
      }
    } else return false; //leave to other handlers
    return true;
  }

  bool broadcastHandler(const String& level, const String& value) { // not sure what to use for but will prob get something...
    LN.logf("BROADCAST", LoggerNode::INFO, "Level %s: %s", level.c_str(), value.c_str());
    return true;
  }
  // REMEMBER: It is not possible to execute delay() or yield() from an asynchronous callback.
  void onHomieEvent(const HomieEvent& event) {
    static Blinky* pre;
    static int prevPixel = -1;
    lastEvent = &event; //can do this? or fall outta scope badd?
    switch(event.type) {
      case HomieEventType::STANDALONE_MODE: break;
      case HomieEventType::CONFIGURATION_MODE:
        // likely some shitty error from malformed config, attempt to restore
        // keep config file backups on spiffs, check for last valid one
        // and/or ask swarm, and/or etc
        pre = new Blinky(RGBW, 125);
        pre->blink("yellow", 2);
        pre->blink("orange", 2);
        pre->blink("white", 1);
        pre->blink("yellow", 1);
        delete pre;
        break;
      case HomieEventType::NORMAL_MODE:
        pre = new Blinky(RGBW, 125);
        pre->blink("green", 2);
        pre->color("white");
        delete pre;
        break;

      case HomieEventType::OTA_STARTED:
        updater["HomieOTA"]->onStart(); break;
      case HomieEventType::OTA_PROGRESS:
        updater["HomieOTA"]->onTick(event.sizeDone, event.sizeTotal); break;
      case HomieEventType::OTA_FAILED:
        updater["HomieOTA"]->onError(0); break;
      case HomieEventType::OTA_SUCCESSFUL:
        updater["HomieOTA"]->onEnd(); break;

      case HomieEventType::ABOUT_TO_RESET:
        Homie.setIdle(false);
        // b->gradient("yellow", "red");
        // homieDelay(100);
        Homie.setIdle(true);
        break;

      case HomieEventType::WIFI_CONNECTED:
        b->gradient("blue", "green");
        break; // normal mode, can use event.ip, event.gateway, event.mask
      case HomieEventType::WIFI_DISCONNECTED:
        b->gradient("blue", "red");
        disconnects++;
        break; // normal mode, can use event.wifiReason
      case HomieEventType::MQTT_READY:
        // if(mqttFirstConnect) {
        b->gradient("orange", "green");
          // mqttFirstConnect = false;
        // }
        break;
      case HomieEventType::MQTT_DISCONNECTED:
        b->gradient("orange", "red");
        break; // can use event.mqttReason

      case HomieEventType::MQTT_PACKET_ACKNOWLEDGED: break; // MQTT packet with QoS > 0 is acknowledged by the broker, can use event.packetId
      case HomieEventType::READY_TO_SLEEP: break; // After calling prepareToSleep() the event is triggered when MQTT is disconnected
    }
  }
};
