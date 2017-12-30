#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <Homie.h>
#include <WiFiUdp.h>
#include <ArtnetnodeWifi.h>
// #include <NeoPixelBus.h>
#include <NeoPixelBrightnessBus.h>
// #include <Timer.h>
#include <Ticker.h>

// D1=5, D2=4, D3=0, D4=2  D0=16, D55=14, D6=12, D7=13, D8=15
#define LED_PIN D1
#define ARTNET_PORT 6454

#define DMX_FUNCTION_CHANNELS 8
#define CH_DIMMER 1
#define CH_STROBE 2
#define CH_STROBE_CURVES 3
#define CH_FUNCTIONS 7

HomieSetting<long> cfg_log_artnet("log_artnet", 				"log level for incoming artnet packets");
HomieSetting<bool> cfg_log_to_serial("log_to_serial", 	"whether to log to serial");
HomieSetting<bool> cfg_log_to_mqtt("log_to_mqtt", 			"whether to log to mqtt");
HomieSetting<long> cfg_strobe_hz_min("strobe_hz_min", 						"lowest strobe frequency");
HomieSetting<long> cfg_strobe_hz_max("strobe_hz_max", 						"highest strobe frequency");
HomieSetting<long> cfg_strips(		"strips", 						"number of LED strips being controlled");
HomieSetting<long> cfg_pin(				"led_pins", 					"pin to use for LED strip control"); // should be array. D1 = 5 / GPIO05
HomieSetting<long> cfg_count(			"led_count", 					"number of LEDs in strip"); // rework for multiple strips
HomieSetting<long> cfg_bytes(			"bytes_per_pixel", 		"3 for RGB, 4 for RGBW");
HomieSetting<long> cfg_universes(	"universes", 					"number of DMX universes");
HomieSetting<long> cfg_start_uni(	"starting_universe", 	"index of first DMX universe used");
HomieSetting<long> cfg_start_addr("starting_address", 	"index of beginning of strip, within starting_universe."); // individual pixel control starts after x function channels offset (for strobing etc)

uint8_t bytes_per_pixel;
uint16_t led_count;
// uint8_t ch_strobe_last_value = 0;
// Timer timer_strobe_each;
struct busState {
	bool shutterOpen;
	NeoPixelBrightnessBus<NeoGrbwFeature,  NeoEsp8266BitBang800KbpsMethod> *bus;	// this way we can (re!)init off config values, post boot
	uint8_t brightness;
	Ticker timer_strobe_each;
	Ticker timer_strobe_on_for;
	float onFraction;
	uint16_t onTime;
	uint8_t ch_strobe_last_value;
	// uint8_t idx;
	// busState(): shutterOpen(true) {}
};

WiFiUDP udp;
ArtnetnodeWifi artnet;
// XXX didnt get Uart mode working, Dma works but iffy with concurrent bitbanging - stick to bang?  +REMEMBER: DMA and UART must ->Begin() after BitBang...
NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod> *bus[4] = {};	// this way we can (re!)init off config values, post boot
NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> *busW[4] = {};	// this way we can (re!)init off config values, post boot
busState* buses = new busState[4]();

void setupOTA();
void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);
void shutterClose(uint8_t idx);
void shutterOpen(uint8_t idx);
// void shutterOpen();

void setup() {
	Homie.disableResetTrigger(); Homie_setFirmware("artnet", "1.0.1");
	cfg_strips.setDefaultValue(1); cfg_pin.setDefaultValue(LED_PIN); cfg_start_uni.setDefaultValue(1); cfg_start_addr.setDefaultValue(1);

	Serial.begin(115200);
	Homie.setup();
	// Homie.enableLogging(cfg_log_to_serial.get()); // gone after homie update, it seems....
	bytes_per_pixel = cfg_bytes.get(); led_count = cfg_count.get();
		
	// if(bus) delete bus;
	if(bytes_per_pixel == 3) {
		// bus[0] = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D1);
		// bus[1] = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
	}
	else if(bytes_per_pixel == 4){
		// buses[0] = new busState();
		buses[0].bus = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D1);
		buses[0].shutterOpen = true;
		buses[0].onFraction = 5;
		buses[0].ch_strobe_last_value = 0;
		// buses[1] = new busState();
		buses[1].bus = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
		buses[1].shutterOpen = true;
		buses[1].onFraction = 5;
		buses[1].ch_strobe_last_value = 0;
		// busW[0] = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D1);
		// busW[1] = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
		// busW[2] = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count / 2, D3);
	}
	for(int i = 0; i < 4; i++) {
		if(buses[i].bus) buses[i].bus->Begin();
		// if(bus[i]) bus[i]->Begin();
		// if(busW[i]) busW[i]->Begin();
	}

  artnet.setName(Homie.getConfiguration().name);
  artnet.setNumPorts(cfg_universes.get());
	artnet.setStartingUniverse(cfg_start_uni.get());
	// XXX watchout for if starting universe is 0
	uint8_t uniidx = cfg_start_uni.get() - 1;
	for(int i=uniidx; i < uniidx + cfg_universes.get(); i++) artnet.enableDMXOutput(i);
	artnet.enableDMXOutput(0); artnet.enableDMXOutput(1); artnet.enableDMXOutput(2); 
	artnet.enableDMXOutput(3);
	artnet.begin();
	artnet.setArtDmxCallback(flushNeoPixelBus);
  udp.begin(ARTNET_PORT);

	setupOTA();

	Homie.getLogger() << "All prepped n ready to break!!!" << endl;
}


void loopArtNet() {
  uint16_t code = artnet.read();
  switch(code) {
    case OpDmx:  if(cfg_log_artnet.get() >= 2) Homie.getLogger() << "DMX "; break;
    case OpPoll: if(cfg_log_artnet.get() >= 1) Homie.getLogger() << "Art Poll Packet"; break;
	}
}

void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
	if(universe < cfg_start_uni.get() || universe >= cfg_start_uni.get() + cfg_universes.get()) return;

	// if(cfg_log_artnet.get() >= 1) Homie.getLogger() << universe;
	uint8_t* functions = &data[-1];  // take care of DMX ch offset... does offset like this actually work lol?
	// uint16_t pixel = 512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
	uint16_t pixel = 0;

	uint8_t busidx = universe - cfg_start_uni.get(); // XXX again watch out for uni 0...

	for(uint16_t t = DMX_FUNCTION_CHANNELS; t < length; t += bytes_per_pixel) { // loop each pixel (4 bytes) in universe
		if(bytes_per_pixel == 3) {
			RgbColor color 	= RgbColor(data[t], data[t+1], data[t+2]);
			// bus[busidx]->SetPixelColor(pixel, color);
		} else if(bytes_per_pixel == 4) { 
			RgbwColor color = RgbwColor(data[t], data[t+1], data[t+2], data[t+3]);
			// busW[busidx]->SetPixelColor(pixel, color);
			buses[busidx].bus->SetPixelColor(pixel, color);
		} pixel++;
	} // GLOBAL FUNCTIONS:
	if(functions[CH_STROBE]) {
		if(functions[CH_STROBE] != buses[busidx].ch_strobe_last_value) { // reset timer for new state
			// uint16_t strobePeriod = 1000 / (float)(functions[CH_STROBE] / 255)   // 1 = 1 hz, 255 = 10 hz, to test
			uint8_t hzMin = cfg_strobe_hz_min.get(); // should these be setable through dmx control ch?
			uint8_t hzMax = cfg_strobe_hz_max.get();
			float hz = ((hzMax - hzMin) * (functions[CH_STROBE] - 1) / (255 - 1)) + hzMin;
			uint16_t strobePeriod = 1000 / hz;   // 1 = 1 hz, 255 = 10 hz, to test
			if(functions[CH_STROBE_CURVES]) { //use non-default fraction
				// buses[busidx].onFraction = ...
			}
			buses[busidx].onTime = strobePeriod / buses[busidx].onFraction; // arbitrary default val. Use as midway point to for period control >127 goes up, < down
			// timer_strobe_on_for.after(onTime, shutterClose);
			// timer_strobe_on_for.once_ms<uint8_t>(onTime, [](uint8_t idx) {
			buses[busidx].timer_strobe_on_for.once_ms<uint8_t>(buses[busidx].onTime, shutterClose, busidx);
			// buses[busidx].timer_strobe_on_for.once_ms<uint8_t>(onTime, shutterClose, 0);
				// }, busidx);
			// buses[busidx].timer_strobe_each.attach_ms<uint8_t>(strobePeriod, [&](uint8_t idx) {
			buses[busidx].timer_strobe_each.attach_ms<uint8_t>(strobePeriod, shutterOpen, busidx);
				// buses[idx].shutterOpen = true;
				// // if(buses[idx]) {
				// 	// buses[idx].bus->SetBrightness(functions[CH_DIMMER]);
				// 	// buses[idx].bus->Show();
				// // } else
				// if(buses[idx].bus) {
				// 	buses[idx].bus->SetBrightness(buses[idx].brightness);
				// 	buses[idx].bus->Show();
				// }
				// buses[idx].timer_strobe_on_for.once_ms<uint8_t>(onTime, shutterClose, idx);
				// }, busidx);
			// timer_strobe.every(strobeTime, shutterOpen);
			buses[busidx].shutterOpen = true;
		} else { // handle strobe logic
		}
		buses[busidx].ch_strobe_last_value = functions[CH_STROBE];	
		// ch_strobe_last_value = functions[CH_STROBE];
	}
	// if(bus[busidx]) {
	// 	if(shutterOpen) bus[busidx]->SetBrightness(functions[CH_DIMMER]);
	// 	bus[busidx]->Show();
	// } else 
		if(buses[busidx].bus)  {
		// Homie.getLogger() << functions[CH_DIMMER] << " ";
		buses[busidx].brightness = functions[CH_DIMMER];
		if(buses[busidx].shutterOpen) buses[busidx].bus->SetBrightness(functions[CH_DIMMER]);
		buses[busidx].bus->Show();
	}
	// if(bus[busidx]) bus[busidx]->Show();
	// else if(busW[busidx]) busW[busidx]->Show();
}

void loop() {
	// timer_strobe_on_for.update();
	// timer_strobe_each.update();
	loopArtNet();

  ArduinoOTA.handle();
	Homie.loop();

	// yield(); // should use?
}

// void shutterOpen() {
//
// }
void shutterClose(uint8_t idx) {
	buses[idx].shutterOpen = false;
	// if(buses[idx]) {
		// buses[idx].bus->SetBrightness(0);
		// buses[idx].bus->Show();
	// } else if(busW[idx]) {
	// } else 
	if(buses[idx].bus) {
		buses[idx].bus->SetBrightness(0);
		buses[idx].bus->Show();
		}
}
void shutterOpen(uint8_t idx) {
	buses[idx].shutterOpen = true;
	// if(buses[idx]) {
		// buses[idx].bus->SetBrightness(functions[CH_DIMMER]);
		// buses[idx].bus->Show();
	// } else
	if(buses[idx].bus) {
		buses[idx].bus->SetBrightness(buses[idx].brightness);
		buses[idx].bus->Show();
	}
	buses[idx].timer_strobe_on_for.once_ms<uint8_t>(buses[idx].onTime, shutterClose, idx);
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

