#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
/* #include <Timer.h> */
#include <Homie.h>
#include <WS2812FX.h>
#include <string.h>

#define WSout 5  // Pin D1
#define WSbit (1<<WSout)
#define ARTNET_DATA 0x50
#define ARTNET_POLL 0x20
#define ARTNET_POLL_REPLY 0x21
#define ARTNET_PORT 6454
#define ARTNET_HEADER 17

WiFiUDP udp;
uint8_t uniData[514]; uint16_t uniSize; uint8_t hData[ARTNET_HEADER + 1];
uint8_t net = 0, universe = 0, subnet = 0;

/* Timer t; */
/* HomieNode mqttLog("artnet", "log"); */
HomieNode brightnessNode("brightness", "scale");

#define LED_COUNT 144
#define LED_PIN D1
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

String cmd = "";               // String to store incoming serial commands
bool cmd_complete = false;  // whether the command string is complete

void loopArtNet();
void printModes();
void setupOTA();
void process_command();


void loop() {
  ArduinoOTA.handle();
	Homie.loop();

	loopArtNet();

	ws2812fx.service();
  /* #if defined(__AVR_ATmega32U4__) */
  serialEvent();
  /* #endif */
  if(cmd_complete) process_command();

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
	brightnessNode.advertise("brightness").settable(brightnessHandler);
	Homie.setup();
	/* mqttLog.setProperty("log").send("test, setting up"); */
	brightnessNode.setProperty("brightness").send("100");
	

	ws2812fx.init();
  ws2812fx.setBrightness(7);
  ws2812fx.setSpeed(200);
  ws2812fx.setColor(0x2A7BFF);
  ws2812fx.setMode(FX_MODE_STATIC);
	ws2812fx.start();
	printModes();

  /* t.every(5000, loopFX);  //update mode */

  udp.begin(ARTNET_PORT); pinMode(WSout, OUTPUT);
	setupOTA(); 

	Homie.getLogger() << "All prepped n ready to break!!!" << endl;

}

void loopFX() {
	ws2812fx.setMode((ws2812fx.getMode() + 1) % ws2812fx.getModeCount());
	Serial.printf("FX Mode %u ", ws2812fx.getMode());
}

// Checks received command and calls corresponding functions.
void process_command() {
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

void serialEvent() {	// Reads new input from serial to cmd string. Command is completed on \n
  while(Serial.available()) {
    char inChar = (char) Serial.read();
    if(inChar == '\n') cmd_complete = true;
    else cmd += inChar;
} }


void loopArtNet() {
  if (!udp.parsePacket()) return;
	udp.read(hData, ARTNET_HEADER + 1); //Serial.print((char*)hData);
	/* if (strncmp((char*)hData, "Art-Net", 7)) { */
	if (hData[0] == 'A' && hData[1] == 'r' && hData[2] == 't' && hData[3] == '-' && hData[4] == 'N' && hData[5] == 'e' && hData[6] == 't') {
		if (hData[8] == 0x00 && hData[9] == ARTNET_DATA && hData[15] == net) {
			if (hData[14] == (subnet << 4) + universe) { // UNIVERSE
				uniSize = (hData[16] << 8) + (hData[17]);
				udp.read(uniData, uniSize);  sendWS();
	} } }
}

void ICACHE_FLASH_ATTR sendWS() {
  uint32_t writeBits; uint8_t bitMask, time;
  os_intr_lock();
  for (uint16_t t = 0; t < uniSize; t++) { 	// outer loop counting bytes
    bitMask = 0x80; while (bitMask) { 			// time=0ns : start by setting bit on
      time = 4; while (time--) WRITE_PERI_REG(0x60000304, WSbit);  		// do ON bits // T=0
      if (uniData[t] & bitMask) writeBits = 0;  		// if this is a '1' keep the on time on for longer, so dont write an off bit
      else 											writeBits = WSbit;  // else it must be a zero, so write the off bit !

      time = 4; while (time--) WRITE_PERI_REG(0x60000308, writeBits); // do OFF bits // T='0' time 350ns
      time = 6; while (time--) WRITE_PERI_REG(0x60000308, WSbit);  		// switch all bits off  T='1' time 700ns
      bitMask >>= 1; 					//end of bite write time=1250ns
	} }; os_intr_unlock();
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

