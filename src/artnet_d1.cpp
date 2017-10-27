#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>

#include <Homie.h>
/* HomieNode mqttLog("artnet", "log"); */
HomieNode brightnessNode("brightness", "scale");

// MANUAL straight DMX->BITBANG method
#include <WiFiUdp.h>
#define WSout 5  // Pin D1
#define WSbit (1<<WSout)
#define ARTNET_DATA 0x50
#define ARTNET_POLL 0x20
#define ARTNET_POLL_REPLY 0x21
#define ARTNET_PORT 6454
#define ARTNET_HEADER 17
WiFiUDP udp;
uint8_t uniData[514]; uint16_t uniSize; uint8_t hData[ARTNET_HEADER + 1];
uint8_t net = 0, subnet = 0, universe = 1, universe2 = 2;


#define LED_COUNT 144
#define LED_PIN D1

// #include <Timer.h>
/* Timer t; */
#include <string.h>
String cmd = "";               // String to store incoming serial commands
bool cmd_complete = false;  // whether the command string is complete
#include <WS2812FX.h>
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);


// #include "WS2812w.h"
// // #include <WS2812.h>
// #define SK_COUNT 128
// #define SK_PIN 5
// WS2812W LED(LED_COUNT);
// cRGB value;
// byte intensity;
// byte sign;

#include <NeoPixelBus.h>
#define colorSaturation 128
NeoPixelBus<NeoRgbwFeature, NeoEsp8266BitBang800KbpsMethod> neoPixelStrip(LED_COUNT, LED_PIN);
void testNeoPixelBus();


void loopArtNet();
void printModes();
void setupOTA();
void process_command();
void serialEvent();
void sendWS();
void flushNeoPixelBus();


void loop() {
  ArduinoOTA.handle();
	Homie.loop();
	loopArtNet();


	// for(uint32_t t = 0; t < uniSize; t++) {
	// 	// XXX read pixel value from incoming artnet...
	// 	value.b = 0; value.g = 0; value.r = 0; value.w = 0;
	// 	LED.set_crgb_at(i, value);
	// } LED.sync();

	// for(uint32_t i = 0; i < SK_COUNT; i++) {
	// 	// value.b = 0; value.g = 0; value.r = 0; value.w = i;
	// 	value.b = 0; value.g = 0; value.r = i;
	// 	LED.set_crgb_at(i, value);
	// } LED.sync();

	// testNeoPixelBus();
	// if (animations.IsAnimating()) { animations.UpdateAnimations(); neoPixelStrip.Show();
	// } else { Serial.println("Setup Next Set..."); SetupAnimationSet(); }



	// ws2812fx.service();
  /* #if defined(__AVR_ATmega32U4__) */
  // serialEvent();
  /* #endif */
  // if(cmd_complete) process_command();

	/* t.update(); */
}

int brightnessHandler(const HomieRange& range, const String& value) {
	Homie.getLogger() << "light handler!" << endl;
	uint8_t b = (uint8_t) value.substring(2, value.length()).toInt();
	ws2812fx.setBrightness(b);

}

void setup() {
  Serial.begin(115200);
	Homie.disableResetTrigger(); Homie_setFirmware("artnet", "1.0.1");
	/* mqttLog.advertise("log"); */
	// brightnessNode.advertise("brightness").settable(brightnessHandler);
	Homie.setup();
	/* mqttLog.setProperty("log").send("test, setting up"); */
	// brightnessNode.setProperty("brightness").send("100");

	neoPixelStrip.Begin(); neoPixelStrip.Show();
	

	// LED.setOutput(SK_PIN);
	// LED.setColorOrderGRBW();
	// intensity = 0; sign = 1;


	// ws2812fx.init();
  // ws2812fx.setBrightness(7); ws2812fx.setSpeed(200); ws2812fx.setColor(0x2A7BFF); ws2812fx.setMode(FX_MODE_STATIC);
	// ws2812fx.start();
	// printModes();

  /* t.every(5000, loopFX);  //update mode */

  udp.begin(ARTNET_PORT); pinMode(WSout, OUTPUT);
	setupOTA(); 

	Homie.getLogger() << "All prepped n ready to break!!!" << endl;
}


void loopArtNet() {
  if (!udp.parsePacket()) return;
	udp.read(hData, ARTNET_HEADER + 1); //Serial.print((char*)hData);
	/* if (strncmp((char*)hData, "Art-Net", 7)) { */
	if (hData[0] == 'A' && hData[1] == 'r' && hData[2] == 't' && hData[3] == '-' && hData[4] == 'N' && hData[5] == 'e' && hData[6] == 't') {
		if (hData[8] == 0x00 && hData[9] == ARTNET_DATA && hData[15] == net) {
			if ((hData[14] == (subnet << 4) + universe) || (hData[14] == (subnet << 4) + universe2)) { // UNIVERSE
				uniSize = (hData[16] << 8) + (hData[17]);
				udp.read(uniData, uniSize);
				// sendWS();
				flushNeoPixelBus();
				// if (Homie.getOption('logartnet')) Serial.println('DMX packet received'); // something like this
	} } }
}

void ICACHE_FLASH_ATTR sendWS() {
  uint32_t writeBits; uint8_t bitMask, time;
  os_intr_lock();
  for(uint16_t t = 0; t < uniSize; t++) { 	// loop each byte in universe
    // bitMask = 0x80; while (bitMask) { 			// loop bits in byte (bitMask) // time=0ns
    for(bitMask = 0x80; bitMask; bitMask >>= 1) { 			// loop bits in byte (bitMask) // time=0ns
      time = 4; while(time--) WRITE_PERI_REG(0x60000304, WSbit);  		 // write ON bits // T=0
      if (uniData[t] & bitMask) writeBits = 0; else writeBits = WSbit; // keep ON (= write 0) if true, else prep OFF

      time = 4; while(time--) WRITE_PERI_REG(0x60000308, writeBits); 	// write OFF bits, zeropulse at time=350ns
      time = 6; while(time--) WRITE_PERI_REG(0x60000308, WSbit);  		// switch all bits off  T='1' time 700ns
      // bitMask >>= 1; 					// shift bitmask // end of bite write time=1250ns
	} }; os_intr_unlock();
}

void flushNeoPixelBus() {
	uint16_t pixel = 0;
  for(uint16_t t = 0; t < uniSize; t += 4) { 	// loop each pixel (4 bytes) in universe
		RgbwColor color = RgbwColor(uniData[t], uniData[t+1], uniData[t+2], uniData[t+3]);
		neoPixelStrip.SetPixelColor(pixel, color);
		pixel++;
	}
	neoPixelStrip.Show();
}

void setupOTA() {
  ArduinoOTA.setHostname("d1");
  /* ArduinoOTA.onProgress([](unsigned int p, unsigned int tot) { Serial.printf("%u%%\r", (p / (tot / 100))); }); */
  ArduinoOTA.onProgress( [](unsigned int p, unsigned int tot) { Serial.printf("OTA updating  -  %u%%\r", p / (tot / 100)); } );
  ArduinoOTA.onError( [](ota_error_t error) {  Serial.printf("OTA Error[%u]: ", error); 
		switch(error) { 
			case OTA_AUTH_ERROR:		Serial.println("Auth Failed"); 		break;
			case OTA_BEGIN_ERROR:		Serial.println("Begin Failed");		break;
			case OTA_CONNECT_ERROR:	Serial.println("Connect Failed");	break;
			case OTA_RECEIVE_ERROR:	Serial.println("Receive Failed");	break;
			case OTA_END_ERROR:			Serial.println("End Failed");			break;
	}	}); ArduinoOTA.begin();
}

void testNeoPixelBus() {
    Serial.println("Colors R, G, B, W...");

    neoPixelStrip.SetPixelColor(0, red); neoPixelStrip.SetPixelColor(1, green); neoPixelStrip.SetPixelColor(2, blue); neoPixelStrip.SetPixelColor(3, white);
    // neoPixelStrip.SetPixelColor(3, RgbwColor(colorSaturation));
    neoPixelStrip.Show();

    delay(1000);
    Serial.println("Off ...");
    neoPixelStrip.SetPixelColor(0, black); neoPixelStrip.SetPixelColor(1, black); neoPixelStrip.SetPixelColor(2, black); neoPixelStrip.SetPixelColor(3, black);
    neoPixelStrip.Show();
    delay(200);
}

void loopFX() {
	ws2812fx.setMode((ws2812fx.getMode() + 1) % ws2812fx.getModeCount());
	Serial.printf("FX Mode %u ", ws2812fx.getMode());
}

void process_command() {	// Checks received command and calls corresponding functions.
  if(cmd[0] == 'b') { 
    uint8_t b = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setBrightness(b);
    Serial.print("Set brightness to: "); Serial.println(ws2812fx.getBrightness());
  } else if(cmd[0] == 's') {
    uint8_t s = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setSpeed(s); 
    Serial.print("Set speed to: "); Serial.println(ws2812fx.getSpeed());
  } else if(cmd[0] == 'm') { 
    uint8_t m = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setMode(m);
    Serial.print("Set mode to: "); Serial.print(ws2812fx.getMode()); Serial.print(" - ");
    Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
  } else if(cmd[0] == 'c') { 
    uint32_t c = (uint32_t) strtol(&cmd.substring(2, cmd.length())[0], NULL, 16);
    ws2812fx.setColor(c); 
    Serial.print("Set color to: 0x");
    if(ws2812fx.getColor() < 0x100000)  Serial.print("0"); 
    if(ws2812fx.getColor() < 0x010000)  Serial.print("0"); 
    if(ws2812fx.getColor() < 0x001000)  Serial.print("0"); 
    if(ws2812fx.getColor() < 0x000100)  Serial.print("0"); 
    if(ws2812fx.getColor() < 0x000010)  Serial.print("0"); 
    Serial.println(ws2812fx.getColor(), HEX);
  }

  cmd = "";              // reset the commandstring
  cmd_complete = false;  // reset command complete
}

void printModes() {	// Prints all available WS2812FX blinken modes.
  Serial.println("Supporting the following modes: ");
  for(int i=0; i < ws2812fx.getModeCount(); i++) {
    Serial.print(i); Serial.print("\t");
    /* Serial.printf("%d\t%u", i, ws2812fx.getModeName(i)); */
    Serial.println(ws2812fx.getModeName(i));
} }

void serialEvent() {	// Reads new input from serial to cmd str. Command is completed on \n
  while(Serial.available()) {
    char inChar = (char) Serial.read();
    if(inChar == '\n') cmd_complete = true;
    else cmd += inChar;
} }
