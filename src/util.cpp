#include "util.h"

const RgbwColor black =   RgbwColor(0, 0, 0, 0);
const RgbwColor white =   RgbwColor(30, 30, 30, 80);
const RgbwColor red =     RgbwColor(170, 25, 10, 20);
const RgbwColor orange =  RgbwColor(150, 70, 10, 20);
const RgbwColor yellow =  RgbwColor(150, 150, 20, 30);
const RgbwColor green =   RgbwColor(20, 170, 40, 20);
const RgbwColor blue =    RgbwColor(20, 60, 180, 20);

const RgbwColor* otaColor;

void blinkStrip(uint8_t numLeds, RgbwColor color, uint8_t blinks) {
  // NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> tempbus(numLeds);
  NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> tempbus(288); // want to clear any previous garbage as well...
  tempbus.Begin(); tempbus.Show();
  delay(100);
  for(int8_t b = 0; b < blinks; b++) {
    tempbus.ClearTo(color); tempbus.Show();
    delay(300);
    tempbus.ClearTo(black); tempbus.Show();
    delay(200);
  }
}

NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> *OTAbus; //(numLeds);
uint16_t leds;
int16_t prevPixel = -1;

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

