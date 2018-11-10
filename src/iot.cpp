#include "iot.h"

HomieNode modeNode("mode",      "mode");
HomieNode statusNode("status",  "log");
HomieNode outputNode("outputs", "io", outputNodeHandler);
HomieNode inputNode("inputs",   "io", inputNodeHandler);
HomieNode colorNode("color",    "fx", colorNodeHandler);

void initHomie() {
  Homie_setFirmware(FW_NAME, FW_VERSION); Homie_setBrand(FW_BRAND);
	Homie.setConfigurationApPassword(FW_NAME);

  // Homie.setLedPin(D2, HIGH); Homie.disableLedFeedback();
  Homie.onEvent(onHomieEvent);
  Homie.setGlobalInputHandler(globalInputHandler).setBroadcastHandler(broadcastHandler);
  // Homie.setSetupFunction(homieSetup).setLoopFunction(homieLoop);

  cfg = new ConfigNode();
  if(analogRead(0) > 100) battery = new BatteryNode(); // hella unsafe, should be a setting
  // if(!cfg.debug.get()) Homie.disableLogging();

	// modeNode.advertise("settings").settable(settingsHandler);
	modeNode.advertiseRange("controls", 1, 12).settable(controlsHandler);
	modeNode.advertiseRange("blend", 1, 12).settable(blendHandler);
  modeNode.advertise("power").settable(powerHandler); //just on/off for Homekit etc...


  colorNode.advertise("color").settable();
  colorNode.advertiseRange("rgbw", 1, 4).settable();
  colorNode.advertiseRange("rgb", 1, 3).settable();
  colorNode.advertiseRange("hsl", 1, 3).settable();
  colorNode.advertise("cmd").settable();

  outputNode.advertise("strip").settable();
  inputNode.advertise("artnet").settable();

  String statusProperties[] = {"freeHeap", "heapFragmentation", "maxFreeBlockSize",
                               "vcc", "fps", "droppedFrames",
                               "dimmer.base", "dimmer.force", "dimmer.out"};
  for(String prop: statusProperties) statusNode.advertise(prop);

	Homie.setup();
}

bool outputNodeHandler(const String& property, const HomieRange& range, const String& value) {
  outputNode.setProperty(property).send(value);
 	LN.logf(__func__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

  if(property == "strip") {
    if(value == "on") pinMode(RX, OUTPUT); //no effect...
    else              pinMode(RX, INPUT_PULLUP); //works
  } else if(property == "serial") {
  } else if(property == "debug") { //toggle using strip for diagnostics (later obv statusleds instead anyways)
  }
  return true;
}

bool inputNodeHandler(const String& property, const HomieRange& range, const String& value) {
  inputNode.setProperty(property).send(value);
 	LN.logf(__func__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

  bool on = (value == "on");
  // future: for registered inputters; if property==inputter.name; pass the message...
  if(property == "artnet") {
    if(on)  artnet->enableDMX();
    else    artnet->disableDMX();
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
  modeNode.setProperty("controls").setRange(range).send(value);
  if(range.index <= f->numChannels)
    f->chOverride[range.index-1] = value.toInt();
  return true;
}
bool blendHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("blend").setRange(range).send(value);
  if(range.index <= f->numChannels)
    f->blendOverride[range.index-1] = value.toFloat();
  return true;
}

RgbwColor globalRgbwColor = RgbwColor(255, 30, 20, 20);
HslColor globalHslColor = HslColor(0.1, 0.42, 0.56);
RgbwColor* globalOutputColor = &globalRgbwColor; //should obv be one and the same later updating both with each...

bool colorNodeHandler(const String& property, const HomieRange& range, const String& value) {
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
//   modeNode.setProperty("settings").send(value);
//   return true; // was thinking interframes, strobehz, dmxhz, log, flipped. but eh so much wooorke
// }

bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value) {
  LN.logf(node.getId(), LoggerNode::INFO, "%s: %s", property.c_str(), value.c_str());

  static std::map<String, std::function<void(const String& value)> > actions; //this'd be koool
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
  lastEvent = *event; //can do this? or fall outta scope badd?
  switch(event.type) {
    case HomieEventType::STANDALONE_MODE: break;
    case HomieEventType::CONFIGURATION_MODE:
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
      homieUpdater.onStart(); break;
    case HomieEventType::OTA_PROGRESS:
      homieUpdater.onTick(event.sizeDone, event.sizeTotal); break;
    case HomieEventType::OTA_FAILED:
      homieUpdater.onError(0); break;
    case HomieEventType::OTA_SUCCESSFUL:
      homieUpdater.onEnd(); break;

    case HomieEventType::ABOUT_TO_RESET:
      Homie.setIdle(false);
      b->gradient("yellow", "red");
      homieDelay(100);
      Homie.setIdle(true);
      break;

    case HomieEventType::WIFI_CONNECTED:
      b->gradient("blue", "green"); // b->color("green"); // b->blink("green", 2, true);
      break; // normal mode, can use event.ip, event.gateway, event.mask
    case HomieEventType::WIFI_DISCONNECTED:
      b->gradient("blue", "red"); // b->color("red"); // blinkStrip(leds, red, 0);
      disconnects++;
      break; // normal mode, can use event.wifiReason
    case HomieEventType::MQTT_READY:
      // if(mqttFirstConnect) {
      b->gradient("orange", "green"); // b->color("orange");
      //   mqttFirstConnect = false;
      // }
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      b->gradient("orange", "red"); // b->color("red");
      break; // can use event.mqttReason

    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED: break; // MQTT packet with QoS > 0 is acknowledged by the broker, can use event.packetId
    case HomieEventType::READY_TO_SLEEP: break; // After calling prepareToSleep() the event is triggered when MQTT is disconnected
  }
}
