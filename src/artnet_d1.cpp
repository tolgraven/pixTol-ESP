#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <Homie.h>
#include <WiFiUdp.h>
#include <ArtnetnodeWifi.h>
#include <NeoPixelBus.h>
#include <Timer.h>
// #include <WS2812FX.h>

#define LED_PIN D1
#define ARTNET_PORT 6454

#define DMX_FUNCTION_CHANNELS 8
#define CH_DIMMER 1
#define CH_STROBE 2
#define CH_STROBE_R 3
#define CH_STROBE_G 4
#define CH_STROBE_B 5
#define CH_STROBE_PIXELS 6
#define CH_FUNCTIONS 7

HomieSetting<bool> cfg_log_artnet("log_artnet", 				"whether to log incoming artnet packets");
HomieSetting<long> cfg_strips(		"strips", 						"number of LED strips being controlled");
HomieSetting<long> cfg_pins(			"led_pins", 					"pins (no separators, '1234') to use for LED strip control"); // should be array. D1 = 5 / GPIO05
HomieSetting<long> cfg_count(			"led_count", 					"number of LEDs in strip"); // rework for multiple strips
HomieSetting<long> cfg_bytes(			"bytes_per_pixel", 		"3 for RGB, 4 for RGBW");
HomieSetting<long> cfg_universes(	"universes", 					"number of DMX universes");
HomieSetting<long> cfg_start_uni(	"starting_universe", 	"index of first DMX universe used");
HomieSetting<long> cfg_start_addr("starting_address", 	"index of beginning of strip, within starting_universe."); // individual pixel control starts after x function channels offset (for strobing etc)

uint8_t bytes_per_pixel;
uint16_t led_count;
// TODO: global function channels:
// 1 Dimmer (so don't have to emulate it in Afterglow...) - could also go past 100? so boosting everything (obvs clipped to 255/255/255/255) - Compensate for fact that Afterglow won't go over 50/50/50/50% when "maxed", needs lightness 100 I guess?
// 2 Strobe 
// 3-5 Strobe Red, Green, Blue
// 6 Strobe functions - per pixel on/off, time-ratio on/off for strobe, halogen emulationish etc
// 7 Function channel, toggling:
// - Automatic light bleed between pixels?
// - Automatic "blur" / afterglow (keep track of past few states for pixel and blend in like)
// - Dithering like just small randomness so esp strobe still looks eh, organic or something
// - Gamma adjust from pixelbus, toggle?
// - neopixelbus - RotateLeft/Right ShiftLeft/Right? for direct sweeps/animations of current state however it was reached... would def allow cool results when activated over dmx
//   						 - the NeoBuffer/NeoVerticalPriteSheet/NeoBitmapFile stuff?
// - random other shit. WS2812FX etc?  // - set as slave 

WiFiUDP udp;
ArtnetnodeWifi artnet;
// XXX how get around having to init RGB or GRBW hardcoded?
// NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> *bus = NULL;	// this way we can (re!)init off config values, post boot
// NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> *bus[2] = {};	// this way we can (re!)init off config values, post boot
// use pointer array to be able to store several, I guess (for different pins/universes, if that's actually proper feasible)
// WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
Timer timer;


void setupOTA();
void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);

// uint8_t start_universe, universes, last_uni;

void setup() {
	Homie.disableResetTrigger(); Homie_setFirmware("artnet", "1.0.1");
	cfg_strips.setDefaultValue(1); cfg_pins.setDefaultValue(LED_PIN); cfg_start_uni.setDefaultValue(1); cfg_start_addr.setDefaultValue(1);

	Homie.setup();
	Homie.getLogger() << "Homie inited, ";
	bytes_per_pixel = cfg_bytes.get(); //led_count = cfg_count.get();
	// universes = cfg_universes.get();
		
	// if(bus) delete bus; // 'enabled by default' warning, so nuke
	// XXX split cfg_pins and get from there... use array of pointers for buses
	// bus = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(cfg_count.get(), 5);
	// bus[1] = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(cfg_count.get(), 6);
	// bus->Begin();
	// bus[1]->Begin();
	
  artnet.setName(Homie.getConfiguration().name);
  artnet.setNumPorts(cfg_universes.get()); 	// wrong obvs since we have 1 strip, 2 unis.,..
	artnet.setStartingUniverse(cfg_start_uni.get());
	artnet.enableDMXOutput(0); artnet.enableDMXOutput(1); 
	artnet.begin();
	artnet.setArtDmxCallback(flushNeoPixelBus);

  udp.begin(ARTNET_PORT);

	setupOTA(); 

	Homie.getLogger() << "setup finished" << endl;
}

void logPixels() {
	// size_t bufsize = bus->PixelsSize();
	// uint8_t* pixels = bus->Pixels();
	// for(size_t p = 0; p < bufsize; ++p)  Serial.printf("%d ", pixels[p]);
}


void loopArtNet() {
  uint16_t code = artnet.read();
  switch(code) {
    case OpDmx:  if(cfg_log_artnet.get()) Homie.getLogger() << "DMX "; break;
    case OpPoll: if(cfg_log_artnet.get()) Homie.getLogger() << "Art Poll Packet"; break;
	}
}

void flushNeoPixelBus(uint16_t universe, uint16_t len, uint8_t sequence, uint8_t* data) {
	uint8_t* functions = &data[1];  // take care of DMX ch offset...
	if(functions[CH_STROBE]) {
		// do that shit, make use of strobe R, G, B and PIXELS etc. Most imporant thing is just keeping the timing on-microcontroller tho, less about throughput per se hey
	}
	uint16_t pixel = 0;

  for(uint16_t t = DMX_FUNCTION_CHANNELS; t < len; t += bytes_per_pixel) { // loop each pixel (4 bytes) in universe
		if(bytes_per_pixel == 3) {
			RgbColor color 	= RgbColor(data[t], data[t+1], data[t+2]);
			// bus[universe - 1]->SetPixelColor(pixel, color); // XXX not hardcode universe to start at 1 (map unis to buses, not straight index)
			// bus->SetPixelColor(pixel, color); // XXX not hardcode universe to start at 1 (map unis to buses, not straight index)
		} else if(bytes_per_pixel == 4) { 
			RgbwColor color = RgbwColor(data[t], data[t+1], data[t+2], data[t+3]);
			// bus->SetPixelColor(pixel, color);
		} pixel++;
	}
	if(functions[CH_DIMMER] != 255) {
		// scale fucking everything. But also should implement above-100 gain that'd be cool... Maybe reg dimmer and then part of ch7 or 8 or whatever is extra gain? hmm

	}
	// bus[universe - 1]->Show();
	// bus->Show();
}

void loop() {
  ArduinoOTA.handle();
	Homie.loop();
	loopArtNet();

	// if(last_uni == universes) {
	// 	bus->Show();
	// 	last_uni = 0;
	// }

	// timer.update();
	// yield(); // should use?
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

