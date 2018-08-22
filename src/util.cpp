#include "util.h"

const RgbwColor* otaColor;

ArtnetnodeWifi artnet;

HomieNode modeNode("mode", "mode");
HomieNode statusNode("status", "log");
HomieNode outputNode("outputs", "io", outputNodeHandler);
HomieNode inputNode("inputs",   "io", inputNodeHandler);


bool outputNodeHandler(const String& property, const HomieRange& range, const String& value) {
  outputNode.setProperty(property).send(value);
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

  if(property == "strip") {
    if(value == "on") pinMode(RX, OUTPUT);
    else              pinMode(RX, INPUT_PULLUP);
  } else if(property == "serial") {
  } return true;
}

bool inputNodeHandler(const String& property, const HomieRange& range, const String& value) {
  inputNode.setProperty(property).send(value);
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

  bool on = value == "on";
  // future: for registered inputters; if property==inputter.name; pass the message...
  if(property == "artnet") {
    if(on)  artnet.enableDMX();
    else    artnet.disableDMX();
  } else if(property == "serial") {
    if(on) { //configure serial port for data input
    } else {
    }
  } else if(property == "mqtt") {
    if(on) {
    } else {
    }
  } else if(property == "dmx") {
    if(on) {
    } else {
    }
  } return true;
}


// put in animation class for strip, or at least pass a bus object lol
void blinkStrip(uint8_t numLeds, RgbwColor color, uint8_t blinks) {
  Strip tempbus("temp", LEDS::RGBW, numLeds);
  delay(100); // XXX use callbacks instead of delays, then mqtt-ota will prob work? Also move this sorta routine into Strip class, or some animation class?
  for(int8_t b = 0; b < blinks; b++) {
    tempbus.SetColor(color);
    yield(); delay(50);
    tempbus.SetColor(colors.black);
    yield(); delay(50);
  }
}

void initArtnet(const String& name, uint8_t numPorts, uint8_t startingUniverse, void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*) ) {
  artnet.setName(name.c_str());
  artnet.setNumPorts(numPorts);
	artnet.setStartingUniverse(startingUniverse);
	for(int i=0; i<numPorts; i++) artnet.enableDMXOutput(i);
	artnet.begin();
	artnet.setArtDmxCallback(callback);
}


Strip* OTAbus;
uint16_t leds;
int prevPixel = -1;

void setupOTA(uint8_t numLeds) {
  ArduinoOTA.setHostname(Homie.getConfiguration().name);
  leds = numLeds;

  ArduinoOTA.onStart([]() {
    Serial.println("\nOTA flash starting...");
    OTAbus = new Strip("OTA strip", LEDS::RGBW, leds);
    OTAbus->Show();
    otaColor = (ArduinoOTA.getCommand() == U_FLASH ? &colors.blue : &colors.yellow);
  });

  ArduinoOTA.onProgress([](unsigned int p, unsigned int tot) {
    uint8_t pixel = p / (tot / leds);
    if(pixel == prevPixel) { // called multiple times each percent of upload...
      // otaColor->Lighten(10);
      return;
    } else {
      // otaColor = (ArduinoOTA.getCommand() == U_FLASH ? &colors.blue : &colors.yellow);
    }
    if(pixel) {
      RgbwColor prevPixelColor;
      OTAbus->driver->GetPixelColor(prevPixel, prevPixelColor);
      prevPixelColor.Darken(25);
      OTAbus->driver->SetPixelColor(prevPixel, prevPixelColor);
    }
    OTAbus->driver->SetPixelColor(pixel, *otaColor);
    OTAbus->Show();
    prevPixel = pixel;
    Serial.printf("OTA updating - %u%%\r", p / (tot/100)); });


  ArduinoOTA.onEnd([]() { blinkStrip(leds, colors.green, 3); });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    switch(error) {
      case OTA_AUTH_ERROR:		Serial.println("Auth Failed"); 		break;
      case OTA_BEGIN_ERROR:		Serial.println("Begin Failed");		break;
      case OTA_CONNECT_ERROR:	Serial.println("Connect Failed");	break;
      case OTA_RECEIVE_ERROR:	Serial.println("Receive Failed");	break;
      case OTA_END_ERROR:			Serial.println("End Failed");			break;
	  } blinkStrip(leds, colors.red, 3); });

  ArduinoOTA.begin();
}


bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value) {
  LN.logf(node.getId(), LoggerNode::INFO, "%s: %s", property.c_str(), value.c_str());

  if(property == "command") {
		if(value == "reset")        Homie.reset();
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
  } else return false;
  return true;
}

bool broadcastHandler(const String& level, const String& value) { // not sure what to use for but will prob get something...
  LN.logf("BROADCAST", LoggerNode::INFO, "Level %s: %s", level.c_str(), value.c_str());
  return true;
}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::STANDALONE_MODE: { break; }
    case HomieEventType::CONFIGURATION_MODE: // blink orange or something?
      // blinkStrip(leds, orange, 5);
      break;
    case HomieEventType::NORMAL_MODE:
      // blinkStrip(leds, blue, 1);
      break;
    case HomieEventType::OTA_STARTED: {
      Serial.println("\nOTA flash starting...");
      LN.logf("OTA", LoggerNode::INFO, "Ota ON OTA ON!!!");
      artnet.disableDMX(); // kill all inputs and outputs, etcetc
      break;
    }
    case HomieEventType::OTA_PROGRESS: { // can use event.sizeDone and event.sizeTotal
      uint8_t pixel = event.sizeDone / (event.sizeTotal / leds);
      // if(pixel == prevPixel) break; // called multiple times each percent of upload...
      Serial.printf("OTA updating - %u%%\r", event.sizeDone / (event.sizeTotal / leds));
      break;
    }
    case HomieEventType::OTA_FAILED: {
      Homie.setIdle(false);
      LN.logf("OTA", LoggerNode::WARNING, "OTA FAILED");
      // blinkStrip(leds, red, 2);
      Homie.setIdle(true);
      break;
    }
    case HomieEventType::OTA_SUCCESSFUL: {
      Homie.setIdle(false);
      LN.logf("OTA", LoggerNode::INFO, "OTA GOODE");
      // blinkStrip(leds, green, 1);
      Homie.setIdle(true);
      break;
    }
    case HomieEventType::ABOUT_TO_RESET: // blinkStrip(leds, orange, 0);
      break;
    case HomieEventType::WIFI_CONNECTED:
      // blinkStrip(leds, green, 1);
      break; // normal mode, can use event.ip, event.gateway, event.mask
    case HomieEventType::WIFI_DISCONNECTED:
      // blinkStrip(leds, red, 0);
      break; // normal mode, can use event.wifiReason
    case HomieEventType::MQTT_READY: break;
    case HomieEventType::MQTT_DISCONNECTED: break; // can use event.mqttReason
    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED: break; // MQTT packet with QoS > 0 is acknowledged by the broker, can use event.packetId
    case HomieEventType::READY_TO_SLEEP: break; // After calling prepareToSleep() the event is triggered when MQTT is disconnected
  }
}

