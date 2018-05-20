#include "util.h"

const RgbwColor black =   RgbwColor(0, 0, 0, 0);
const RgbwColor white =   RgbwColor(30, 30, 30, 80);
const RgbwColor red =     RgbwColor(170, 25, 10, 20);
const RgbwColor orange =  RgbwColor(150, 70, 10, 20);
const RgbwColor yellow =  RgbwColor(150, 150, 20, 30);
const RgbwColor green =   RgbwColor(20, 170, 40, 20);
const RgbwColor blue =    RgbwColor(20, 60, 180, 20);

const RgbwColor* otaColor;

ArtnetnodeWifi artnet;
WiFiUDP udp;

HomieNode modeNode("mode", "mode");

HomieNode outputNode("io", "outputs", outputHandler);
HomieNode inputNode("io", "inputs", inputHandler);

bool outputHandler(const String& property, const HomieRange& range, const String& value) {
  outputNode.setProperty(property).send(value);
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

  if(property.equals("strip")) {
    if(value.equals("on")) {
      pinMode(RX, OUTPUT);
    } else {
      pinMode(RX, INPUT);
    }
  }
  // else if(property.equals("serial")) {
  //
  // }
}

bool inputHandler(const String& property, const HomieRange& range, const String& value) {
  inputNode.setProperty(property).send(value);
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

  if(property.equals("artnet")) {
    if(value.equals("on")) {
      artnet.enableDMX();
      // for(uint8_t i=0; i < cfg->universes.get(); i++) {
      //     artnet.enableDMXOutput(i);
      // }
    } else {
      artnet.disableDMX();
      // for(uint8_t i=0; i < cfg->universes.get(); i++) {
      //     artnet.disableDMXOutput(i);
      // }
    }
  // } else if(property.equals("serial")) {
  //   if(value.equals("on")) { //configure serial port for data input
  //
  //   } else {
  //
  //   }
  // } else if(property.equals("mqtt")) {
  //   if(value.equals("on")) {
  //
  //   } else {
  //
  //   }
  // } else if(property.equals("dmx")) {
  //   if(value.equals("on")) {
  //
  //   } else {
  //
  //   }
  }
	return true;
}


// put in animation class for strip, or at least pass a bus object lol
void blinkStrip(uint8_t numLeds, RgbwColor color, uint8_t blinks) {
  DmaGRBW tempbus(numLeds); // want to clear any previous garbage as well...
  tempbus.Begin(); tempbus.Show();
  delay(100); // XXX use callbacks instead of delays, then mqtt-ota will prob work? Also move this sorta routine into Strip class, or some animation class?
  for(int8_t b = 0; b < blinks; b++) {
    tempbus.ClearTo(color); tempbus.Show();
    delay(100);
    tempbus.ClearTo(black); tempbus.Show();
    delay(100);
  }
}

void initArtnet(const String& name, uint8_t numPorts, uint8_t startingUniverse, void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*) ) {
  artnet.setName(name.c_str());
  artnet.setNumPorts(numPorts);
	artnet.setStartingUniverse(startingUniverse);
	for(uint8_t i=0; i < numPorts; i++) {
      artnet.enableDMXOutput(i);
  }
	artnet.begin();
	artnet.setArtDmxCallback(callback);
}


DmaGRBW *OTAbus;
uint16_t leds;
int prevPixel = -1;

void setupOTA(uint8_t numLeds) {
  ArduinoOTA.setHostname(Homie.getConfiguration().name);
  leds = numLeds;

  ArduinoOTA.onStart([]() {
    Serial.println("\nOTA flash starting...");
    OTAbus = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>(leds);
    OTAbus->Begin(); OTAbus->Show();
    otaColor = (ArduinoOTA.getCommand() == U_FLASH ? &blue : &yellow);
  });

  ArduinoOTA.onProgress([](unsigned int p, unsigned int tot) {
    uint8_t pixel = p / (tot / leds);
    if(pixel == prevPixel) return; // called multiple times each percent of upload...
    if(pixel) {
      RgbwColor prevPixelColor = OTAbus->GetPixelColor(prevPixel);
      prevPixelColor.Darken(80);
      OTAbus->SetPixelColor(prevPixel, prevPixelColor);
    }
    OTAbus->SetPixelColor(pixel, *otaColor);
    OTAbus->Show();
    prevPixel = pixel;
    Serial.printf("OTA updating - %u%%\r", p / (tot/100));
  });

  ArduinoOTA.onEnd([]() {
    blinkStrip(leds, green, 3);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    switch(error) { 
      case OTA_AUTH_ERROR:		Serial.println("Auth Failed"); 		break;
      case OTA_BEGIN_ERROR:		Serial.println("Begin Failed");		break;
      case OTA_CONNECT_ERROR:	Serial.println("Connect Failed");	break;
      case OTA_RECEIVE_ERROR:	Serial.println("Receive Failed");	break;
      case OTA_END_ERROR:			Serial.println("End Failed");			break;
	  }
    blinkStrip(leds, red, 3);
  });

  ArduinoOTA.begin();
}


bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value) {
  Homie.getLogger() << "Received on node " << node.getId() << ": " << property << " = " << value << endl;
  // char* msg = "Received on node ", node.getId(), ": " , property , " = " , value;
  // logNode.setProperty("log").send(msg);
  logNode.setProperty("log").send("Received msg on node: ");
  logNode.setProperty("log").send(node.getId());
  logNode.setProperty("log").send(", property: ");
  logNode.setProperty("log").send(property);
  logNode.setProperty("log").send(", value: ");
  logNode.setProperty("log").send(value);

  if(property.equals("command")) {
		if(value.equals("reset")) Homie.reset();
		else if(value.equals("ledoff")) Homie.disableLedFeedback();
	} else if(property.equals("mode")) {
		if(value.equals("dmx")) {
			// eventually multiple input/output types etc, LTP/HTP/average/prio, specific mergin like
			// animation over serial, DMX FN CH from artnet
			// animation over artnet, fn ch overrides from mqtt

		} else if(value.equals("mqtt")) {

		} else if(value.equals("standalone")) {

		}
  } else {
    return false;
  }
  return true;
}


void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::STANDALONE_MODE: {
      break;
    }
    case HomieEventType::CONFIGURATION_MODE: // blink orange or something?
      // blinkStrip(leds, orange, 5);
      break;
    case HomieEventType::NORMAL_MODE:
      // blinkStatus(blue, 1);
      break;
    case HomieEventType::OTA_STARTED: {
      Serial.println("\nOTA flash starting...");
      logNode.setProperty("log").send("OTA IS ON YO");
      LN.logf("OTA", LoggerNode::INFO, "Ota ON OTA ON!!!");
      // if(buses[0].bus) {
      //     buses[0].bus->ClearTo(blueZ);
      // yield();
      //     buses[0].bus->Show();
      // }
      // if(buses[0].busW) {
      //     buses[0].busW->ClearTo(blue);
      // yield();
      //     buses[0].busW->Show();
      // }

      // OTAbus = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>(5);
      // OTAbus->Begin();
      // OTAbus->Show();
      // otaColor = &blue;
      break;
    }
    case HomieEventType::OTA_PROGRESS: { // can use event.sizeDone and event.sizeTotal
      uint8_t pixel = event.sizeDone / (event.sizeTotal / leds);
      if(pixel == prevPixel) break; // called multiple times each percent of upload...
    //   if(pixel) {
    //     RgbwColor prevPixelColor = OTAbus->GetPixelColor(prevPixel);
    //     prevPixelColor.Darken(80);
    //     OTAbus->SetPixelColor(prevPixel, prevPixelColor);
    //   }
    //   OTAbus->SetPixelColor(pixel, *otaColor);
    //   OTAbus->Show();
    //   prevPixel = pixel;
      Serial.printf("OTA updating - %u%%\r", event.sizeDone / (event.sizeTotal / leds));
    //   // buses[0].busW->SetPixelColor(event.sizeDone / (event.sizeTotal / leds), blue);
    //   // buses[0].busW->Show();
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
      // yield();
      // delay(50);
      Homie.setIdle(true);
      break;
    }
    case HomieEventType::ABOUT_TO_RESET:
      // blinkStrip(leds, orange, 0);
      break;
    case HomieEventType::WIFI_CONNECTED: // normal mode, can use event.ip, event.gateway, event.mask
      // blinkStrip(leds, green, 1);
      break;
    case HomieEventType::WIFI_DISCONNECTED: // normal mode, can use event.wifiReason
      // blinkStrip(leds, red, 0);
      break;
    case HomieEventType::MQTT_READY:
      break;
    case HomieEventType::MQTT_DISCONNECTED: // can use event.mqttReason
      break;
    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED: // MQTT packet with QoS > 0 is acknowledged by the broker, can use event.packetId
      break;
    case HomieEventType::READY_TO_SLEEP: // After calling prepareToSleep() the event is triggered when MQTT is disconnected
      break;
  }
}

