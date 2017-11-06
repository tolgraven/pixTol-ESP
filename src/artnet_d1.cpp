#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>

#include <Homie.h>
HomieSetting<bool> cfg_log_artnet("log_artnet", "whether to log incoming artnet packets");
HomieSetting<long> cfg_strips("strips", "number of LED strips being controlled");
HomieSetting<long> cfg_pin("led_pin", "pin to use for LED strip control"); // should be array. D1 = 5 / GPIO05
HomieSetting<long> cfg_count("led_count", "number of LEDs in strip"); // rework for multiple strips
HomieSetting<long> cfg_bytes("bytes_per_pixel", "3 for RGB, 4 for RGBW");
HomieSetting<long> cfg_universes("universes", "number of DMX universes");
HomieSetting<long> cfg_start_uni("starting_universe", "index of first DMX universe used");
HomieSetting<long> cfg_start_addr("starting_address", "index of beginning of strip, within starting_universe."); // individual pixel control starts after x function channels offset (for strobing etc)
// XXX define num leds as well and automatically work out how to map universes based on bytes_per_pixel, num leds and starting universe?
// or struct with universe-pin/offset mapping?
// want flexibility to both control one strip multiple universes, and multiple strips on one universe...
uint8_t bytes_per_pixel;

// TODO:
// first few channels are DMX control. Definitely:
// ** Strobe 
// ** Dimmer (so don't have to emulate it in Afterglow...) - could also go past 100? so boosting everything (obvs clipped to 255/255/255/255) - Compensate for fact that Afterglow won't go over 50/50/50/50% when "maxed", needs lightness 100 I guess?
// ** Function channel, toggling:
// - Halogen emulation curve...
// - Automatic light bleed between pixels?
// - Automatic "blur" / afterglow (keep track of past few states for pixel and blend in like)
// - Gamma adjust from pixelbus, toggle?
// - neopixelbus
//   - RotateLeft/Right ShiftLeft/Right? for direct sweeps/animations of current state however it was reached... would def allow cool results when activated over dmx
//   - the NeoBuffer/NeoVerticalPriteSheet/NeoBitmapFile stuff?
// - random other shit. WS2812FX etc?
// - set as slave 

// MANUAL straight ARTNET->DMX->BITBANG method
#include <WiFiUdp.h>
// #define WSout 5  // Pin D1
// #define WSbit (1<<WSout)
// #define ARTNET_DATA 0x50
// #define ARTNET_POLL 0x20
// #define ARTNET_POLL_REPLY 0x21
// #define ARTNET_PORT 6454
// #define ARTNET_HEADER 17
// WiFiUDP udp;
// uint8_t uniData[514]; uint16_t uniSize; uint8_t h[ARTNET_HEADER + 1];
uint8_t net = 0, subnet = 0, universe;

uint8_t universes;
uint8_t last_uni;

uint16_t led_count;
#define LED_COUNT 300
#define LED_PIN D1

#include <Timer.h>
Timer t;
// #include <WS2812FX.h>
// WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);


#include <NeoPixelBus.h>
// NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> bus(LED_COUNT, LED_PIN);
NeoPixelBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> *bus = NULL;

#include <ArtnetnodeWifi.h>
ArtnetnodeWifi artnet;

void loopArtNet();
void setupOTA();
void sendWS();
void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);

void logPixels();

void loop() {
  ArduinoOTA.handle();
	Homie.loop();
	// loopArtNet();
	artnet.read();

	if(last_uni == universes) {
		bus->Show();
		last_uni = 0;
	}
	// t.update();
	yield(); // should use?
}


void setup() {
  Serial.begin(115200);
	Homie.disableResetTrigger(); Homie_setFirmware("artnet", "1.0.1");
	cfg_strips.setDefaultValue(1); cfg_pin.setDefaultValue(LED_PIN); cfg_start_uni.setDefaultValue(1); cfg_start_addr.setDefaultValue(1);

	Homie.setup();
	bytes_per_pixel = cfg_bytes.get(); led_count = cfg_count.get(); universe = cfg_start_uni.get();
	// uint8_t universes = (led_count * bytes_per_pixel) / 512; universes = 2;
	universes = cfg_universes.get();
		
	if(bus) delete bus;
	bus = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>(led_count);
	bus->Begin();
	
  artnet.setName(Homie.getConfiguration().name);
  artnet.setNumPorts(2); artnet.setStartingUniverse(1);
	artnet.begin();
	artnet.setArtDmxCallback(flushNeoPixelBus);
  // udp.begin(ARTNET_PORT); pinMode(WSout, OUTPUT);
	setupOTA(); 

	Homie.getLogger() << "All prepped n ready to break!!!" << endl;
}

void logPixels() {
	size_t bufsize = bus->PixelsSize();
	uint8_t* pixels = bus->Pixels();
	for(size_t p = 0; p < bufsize; ++p)  Serial.printf("%d ", pixels[p]);
}


void loopArtNet() {
  uint16_t code = artnet.read();
  switch(code) {
    case OpDmx: // dmx data, let callback handle...
			break;
    case OpPoll:
			Serial.println("Art Poll Packet"); break;
	}

  // if(!udp.parsePacket()) return;
	// udp.read(h, ARTNET_HEADER + 1);
	// if(h[0]=='A' && h[1]=='r' && h[2]=='t' && h[3]=='-' && h[4]=='N' && h[5]=='e' && h[6]=='t') {
	// 	if (h[8] == 0x00 && h[9] == ARTNET_DATA && h[15] == net) {
	// 		for(int uni = universe; uni <= universes; uni++) {
	// 			if (h[14] == (subnet << 4) + uni) { // UNIVERSE
	// 				uniSize = (h[16] << 8) + (h[17]);
	// 				udp.read(uniData, uniSize);
	// 				// sendWS(); 				// manual bitbang method, only for WS2812b RGB
	// 				flushNeoPixelBus(uni); // works with RGBW etc as well... but will still have to run sep controllers per type I guess? still, same-ish codebase = good
	// 				last_uni = uni;
					// if (log_artnet.get()) Serial.println('DMX packet received'); // something like this
		// }	}
	// bus->Show();
	// } } 
}

// void ICACHE_FLASH_ATTR sendWS() {
//   uint32_t writeBits; uint8_t bitMask, time;
//   os_intr_lock();
//   for(uint16_t t = 0; t < uniSize; t++) { 	// loop each byte in universe
//     // bitMask = 0x80; while (bitMask) { 			// loop bits in byte (bitMask) // time=0ns
//     for(bitMask = 0x80; bitMask; bitMask >>= 1) { 			// loop bits in byte (bitMask) // time=0ns
//       time = 4; while(time--) WRITE_PERI_REG(0x60000304, WSbit);  		 // write ON bits // T=0
//       if (uniData[t] & bitMask) writeBits = 0; else writeBits = WSbit; // keep ON (= write 0) if true, else prep OFF
//       time = 4; while(time--) WRITE_PERI_REG(0x60000308, writeBits); 	// write OFF bits, zeropulse at time=350ns
//       time = 6; while(time--) WRITE_PERI_REG(0x60000308, WSbit);  		// switch all bits off  T='1' time 700ns
//       // bitMask >>= 1; 					// shift bitmask // end of bite write time=1250ns
// 	} }; os_intr_unlock();
// }

void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
	uint16_t pixel = 512/bytes_per_pixel * (universe - 1);
  for(uint16_t t = 0; t < length; t += bytes_per_pixel) { // loop each pixel (4 bytes) in universe
		if(bytes_per_pixel == 3) {
			RgbColor color 	= RgbColor(data[t], data[t+1], data[t+2]);
			bus->SetPixelColor(pixel, color);
		} else if(bytes_per_pixel == 4) { 
			RgbwColor color = RgbwColor(data[t], data[t+1], data[t+2], data[t+3]);
			bus->SetPixelColor(pixel, color);
		}
		pixel++;
	}
}

void setupOTA() {
  ArduinoOTA.setHostname(Homie.getConfiguration().name);
  ArduinoOTA.onProgress([](unsigned int p, unsigned int tot) { Serial.printf("OTA updating - %u%%\r", p / (tot/100)); });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("OTA Error[%u]: ", error); switch(error) { 
			case OTA_AUTH_ERROR:		Serial.println("Auth Failed"); 		break;
			case OTA_BEGIN_ERROR:		Serial.println("Begin Failed");		break;
			case OTA_CONNECT_ERROR:	Serial.println("Connect Failed");	break;
			case OTA_RECEIVE_ERROR:	Serial.println("Receive Failed");	break;
			case OTA_END_ERROR:			Serial.println("End Failed");			break;
	}	}); ArduinoOTA.begin();
}

