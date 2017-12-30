#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <Homie.h>
#include <WiFiUdp.h>
#include <ArtnetnodeWifi.h>
#include <NeoPixelBrightnessBus.h>
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
struct busState {
	// bool shutterOpen; // actually do we even want this per bus? obviously for strobe etc this is one unit, no? so one shutter, one timer etc. but then further hax on other end for fixture def larger than one universe...
	// prob just use first connected strip's function channels, rest are slaved?
	NeoPixelBrightnessBus<NeoGrbwFeature,  NeoEsp8266BitBang800KbpsMethod> *bus;	// this way we can (re!)init off config values, post boot
	// uint8_t brightness;
	// Ticker timer_strobe_each;
	// Ticker timer_strobe_on_for;
	// float onFraction;
	// uint16_t onTime;
	// uint8_t ch_strobe_last_value;
	// busState(): shutterOpen(true) {}
};

WiFiUDP udp;
ArtnetnodeWifi artnet;
// XXX didnt get Uart mode working, Dma works but iffy with concurrent bitbanging - stick to bang?  +REMEMBER: DMA and UART must ->Begin() after BitBang...
NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod> *bus[4] = {};	// this way we can (re!)init off config values, post boot
NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> *busW[4] = {};	// this way we can (re!)init off config values, post boot
busState* buses = new busState[4]();
bool shutterOpen = true;
Ticker timer_strobe_predelay;
Ticker timer_strobe_each;
Ticker timer_strobe_on_for;
float onFraction = 7;
uint16_t onTime;
uint8_t ch_strobe_last_value = 0;
uint8_t brightness;

void setupOTA();
void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);
void shutterCloseCallback(); //uint8_t idx);
void shutterOpenCallback(); //uint8_t idx);

void setup() {
	Homie.disableResetTrigger(); Homie_setFirmware("artnet", "1.0.1");
	cfg_strips.setDefaultValue(1); cfg_pin.setDefaultValue(LED_PIN); cfg_start_uni.setDefaultValue(1); cfg_start_addr.setDefaultValue(1);

	Serial.begin(115200);
	Homie.setup();
	// Homie.enableLogging(cfg_log_to_serial.get()); // gone after homie update, it seems....
	bytes_per_pixel = cfg_bytes.get(); led_count = cfg_count.get();
		
	if(bytes_per_pixel == 3) {
		// bus[0] = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D1);
		// bus[1] = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
	}
	else if(bytes_per_pixel == 4){
		buses[0].bus = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D1);
		// buses[0].shutterOpen = true;
		// buses[0].onFraction = 7;
		// buses[0].ch_strobe_last_value = 0;
		buses[1].bus = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
		// buses[1].shutterOpen = true;
		// buses[1].onFraction = 7;
		// buses[1].ch_strobe_last_value = 0;
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
	// bool flushStrobe = false;

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
	if(busidx == 0) { // first strip is master (at least for strobe etc)
		if(functions[CH_STROBE]) {
			if(functions[CH_STROBE] != ch_strobe_last_value) { // reset timer for new state
				// XXX does same stupid thing as ADJ wash when sweeping strobe rate. because it starts instantly I guess. throttle somehow, or offset actual strobe slightly.
				// buses[busidx].timer_strobe_on_for.detach(); // test, tho guess little chance it'd hit inbetween here and later in if body
				// buses[busidx].timer_strobe_each.detach();
				uint8_t hzMin = cfg_strobe_hz_min.get(); // should these be setable through dmx control ch?
				uint8_t hzMax = cfg_strobe_hz_max.get();
				float hz = ((hzMax - hzMin) * (functions[CH_STROBE] - 1) / (255 - 1)) + hzMin;
				uint16_t strobePeriod = 1000 / hz;   // 1 = 1 hz, 255 = 10 hz, to test
				// if(functions[CH_STROBE_CURVES]) { //use non-default fraction
				// 	// buses[busidx].onFraction = ...
				// }
				onTime = strobePeriod / onFraction; // arbitrary default val. Use as midway point to for period control >127 goes up, < down
						// timer_strobe_on_for.once_ms<uint8_t>(onTime, shutterCloseCallback, busidx);
						// shutterOpen = true;
						// for(uint8_t i = 0; i < cfg_universes.get(); i++) {
						// 	buses[i].bus->SetBrightness(brightness);
						// 	buses[i].bus->Show();
						// }


				timer_strobe_predelay.once_ms(10, shutterOpenCallback);
				// timer_strobe_predelay.once_ms(20, []() {
				// 		timer_strobe_on_for.once_ms(onTime, shutterOpenCallback); //, busidx);
				// 		shutterOpen = true;
				// 		for(uint8_t i = 0; i < cfg_universes.get(); i++) {
				// 			buses[i].bus->SetBrightness(brightness);
				// 			buses[i].bus->Show();
				// 		}
				// });
				
				// timer_strobe_each.attach_ms<uint8_t>(strobePeriod, shutterOpenCallback, busidx);
				timer_strobe_each.attach_ms(strobePeriod, shutterOpenCallback); //, busidx);

				// flushStrobe = true;
			}
		} else { // 0, clean up
			timer_strobe_on_for.detach();
			timer_strobe_each.detach();
			shutterOpen = true;
		}
		ch_strobe_last_value = functions[CH_STROBE];
	}
	if(buses[busidx].bus) {
		brightness = functions[CH_DIMMER];
		if(brightness < 8) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
		if(shutterOpen) buses[busidx].bus->SetBrightness(brightness);
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
void shutterCloseCallback() { //uint8_t idx) {
	shutterOpen = false;
	for(uint8_t i = 0; i < cfg_universes.get(); i++) { // flush all at once, don't wait for DMX packets...
		buses[i].bus->SetBrightness(0);
		buses[i].bus->Show();
	}
	// if(buses[idx].bus) {
	// 	buses[idx].bus->SetBrightness(0);
	// 	buses[idx].bus->Show();
	// 	}
}
void shutterOpenCallback() { //uint8_t idx) {
	shutterOpen = true;
	for(uint8_t i = 0; i < cfg_universes.get(); i++) { // flush all at once, don't wait for DMX packets...
		buses[i].bus->SetBrightness(brightness);
		buses[i].bus->Show();
	}
	// if(buses[idx].bus) {
	// 	buses[idx].bus->SetBrightness(brightness);
	// 	buses[idx].bus->Show();
	// }
	timer_strobe_on_for.once_ms(onTime, shutterCloseCallback); //, idx);
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

