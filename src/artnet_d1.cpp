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
#define CH_RELEASE 4
#define CH_FUNCTIONS 7

HomieSetting<long> cfg_log_artnet("log_artnet", 				"log level for incoming artnet packets");
HomieSetting<bool> cfg_log_to_serial("log_to_serial", 	"whether to log to serial");
HomieSetting<bool> cfg_log_to_mqtt("log_to_mqtt", 			"whether to log to mqtt");
HomieSetting<long> cfg_strobe_hz_min("strobe_hz_min", 	"lowest strobe frequency");
HomieSetting<long> cfg_strobe_hz_max("strobe_hz_max", 	"highest strobe frequency");
HomieSetting<long> cfg_strips(		"strips", 						"number of LED strips being controlled");
HomieSetting<bool> cfg_mirror(		"mirror_second_half", "whether to mirror strip back onto itself, for controlling folded strips >1 uni as if one");
HomieSetting<long> cfg_pin(				"led_pins", 					"pin to use for LED strip control"); // should be array. D1 = 5 / GPIO05
HomieSetting<long> cfg_count(			"led_count", 					"number of LEDs in strip"); // rework for multiple strips
HomieSetting<long> cfg_bytes(			"bytes_per_pixel", 		"3 for RGB, 4 for RGBW");
HomieSetting<long> cfg_universes(	"universes", 					"number of DMX universes");
HomieSetting<long> cfg_start_uni(	"starting_universe", 	"index of first DMX universe used");
HomieSetting<long> cfg_start_addr("starting_address", 	"index of beginning of strip, within starting_universe."); // individual pixel control starts after x function channels offset (for strobing etc)

uint8_t bytes_per_pixel;
uint16_t led_count;
struct busState {
	// bool shutterOpen; // actually do we even want this per bus? obviously for strobe etc this is one unit, no? so one shutter, one timer etc. but then further hax on other end for fixture def larger than one universe...  prob just use first connected strip's function channels, rest are slaved?
	NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod> *bus;	// this way we can (re!)init off config values, post boot
	NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> *busW;	// this way we can (re!)init off config values, post boot
};

WiFiUDP udp;
ArtnetnodeWifi artnet;
// XXX didnt get Uart mode working, Dma works but iffy with concurrent bitbanging - stick to bang?  +REMEMBER: DMA and UART must ->Begin() after BitBang...
// NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod> *bus[4] = {};	// this way we can (re!)init off config values, post boot
// NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> *busW[4] = {};	// this way we can (re!)init off config values, post boot
busState* buses = new busState[4]();
bool shutterOpen = true;
Ticker timer_strobe_predelay;
Ticker timer_strobe_each;
Ticker timer_strobe_on_for;
float onFraction = 8;
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
	bytes_per_pixel = cfg_bytes.get();
	led_count = cfg_count.get();
	if(cfg_mirror.get()) led_count *= 2;	// actual leds in strip twice what's specified

	if(bytes_per_pixel == 3) {
		buses[0].bus = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D1);
		// buses[1].bus = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
	}
	else if(bytes_per_pixel == 4){
		buses[0].busW = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D1);
		buses[1].busW = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
		buses[2].busW = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count / 2, D3);
	}
	for(int i = 0; i < 4; i++) {
		if(buses[i].bus)  buses[i].bus->Begin();
		if(buses[i].busW) buses[i].busW->Begin();
	}

  artnet.setName(Homie.getConfiguration().name);
  artnet.setNumPorts(cfg_universes.get());
	artnet.setStartingUniverse(cfg_start_uni.get());
	// XXX watchout for if starting universe is 0
	uint8_t uniidx = cfg_start_uni.get() - 1;
	for(uint8_t i=uniidx; i < uniidx + cfg_universes.get(); i++) artnet.enableDMXOutput(i);
	artnet.enableDMXOutput(0); artnet.enableDMXOutput(1); artnet.enableDMXOutput(2); 
	artnet.enableDMXOutput(3);
	artnet.begin();
	artnet.setArtDmxCallback(flushNeoPixelBus);
  udp.begin(ARTNET_PORT);

	setupOTA();

	Homie.getLogger() << endl << "Setup completed" << endl << endl;
}


void loopArtNet() {
  switch(artnet.read()) { // check opcode for logging purposes, actual logic in setArtDmxCallback function
    case OpDmx:  if(cfg_log_artnet.get() >= 2) Homie.getLogger() << "DMX "; 					 break;
    case OpPoll: if(cfg_log_artnet.get() >= 1) Homie.getLogger() << "Art Poll Packet"; break;
	}
}

void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
	if(universe < cfg_start_uni.get() || universe >= cfg_start_uni.get() + cfg_universes.get()) return; // need to expand with different subnets etc...

	if(cfg_log_artnet.get() >= 1) Homie.getLogger() << universe;
	uint8_t* functions = &data[-1];  // take care of DMX ch offset...
	// uint16_t pixel = 512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
	uint16_t pixel = 0;
	uint8_t busidx = universe - cfg_start_uni.get(); // XXX again watch out for uni 0...

	// for(uint16_t t = DMX_FUNCTION_CHANNELS; t < length; t += bytes_per_pixel) { // loop each pixel (4 bytes) in universe
	for(uint16_t t = DMX_FUNCTION_CHANNELS; t < bytes_per_pixel * cfg_count.get() + DMX_FUNCTION_CHANNELS; t += bytes_per_pixel) { // loop each pixel (4 bytes) in universe
		if(bytes_per_pixel == 3) {
			RgbColor color 	= RgbColor(data[t], data[t+1], data[t+2]);
			if(color.CalculateBrightness() == 0) { // XXX rather, if delta > release then fade? or focus on "attack"/blend?
				color = buses[busidx].bus->GetPixelColor(pixel);
				color.Darken(30); // delta 0-255
				if(color.CalculateBrightness() < 16) color.Darken(16); // avoid bitcrunch
			} else {
				color = RgbColor::LinearBlend(buses[busidx].bus->GetPixelColor(pixel), color, 0.2f);
			}
			buses[busidx].bus->SetPixelColor(pixel, color);

			if(cfg_mirror.get()) buses[busidx].bus->SetPixelColor(2*cfg_count.get() - pixel - 1, color);
		} else if(bytes_per_pixel == 4) { 
			RgbwColor color = RgbwColor(data[t], data[t+1], data[t+2], data[t+3]);
			buses[busidx].busW->SetPixelColor(pixel, color);
			if(cfg_mirror.get()) buses[busidx].busW->SetPixelColor(2*cfg_count.get() - pixel - 1, color);
		} pixel++;
	}
	// if(cfg_mirror.get()) {
	// 	for(uint16_t p = 0; cfg_count.get() / 2)
	// }
	// GLOBAL FUNCTIONS:
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

				timer_strobe_predelay.once_ms(8, shutterOpenCallback); // 1 frame at 40hz = 25 ms so prob dont want to go above  10 before double flush
				timer_strobe_each.attach_ms(strobePeriod, shutterOpenCallback); //, busidx);
			}
		} else { // 0, clean up
			timer_strobe_on_for.detach();
			timer_strobe_each.detach();
			shutterOpen = true;
		}
		ch_strobe_last_value = functions[CH_STROBE];
	}
	brightness = functions[CH_DIMMER];
	if(brightness < 12) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
	if(shutterOpen)  {
		if(buses[busidx].bus) {
			buses[busidx].bus->SetBrightness(brightness);
			buses[busidx].bus->Show();
		}	else if(buses[busidx].busW) {
			buses[busidx].busW->SetBrightness(brightness);
			buses[busidx].busW->Show();
		}
	}
}

void loop() {
	loopArtNet();
  ArduinoOTA.handle();
	Homie.loop();

	// yield(); // should use?
}

void shutterCloseCallback() { //uint8_t idx) {
	shutterOpen = false;
	for(uint8_t i = 0; i < cfg_universes.get(); i++) { // flush all at once, don't wait for DMX packets...
		if(buses[i].bus) {
			buses[i].bus->SetBrightness(0);
			buses[i].bus->Show();
		}	else if(buses[i].busW) {
			buses[i].busW->SetBrightness(0);
			buses[i].busW->Show();
		}
	}
}

void shutterOpenCallback() { //uint8_t idx) {
	shutterOpen = true;
	for(uint8_t i = 0; i < cfg_universes.get(); i++) { // flush all at once, don't wait for DMX packets...
		if(buses[i].bus) {
			buses[i].bus->SetBrightness(brightness);
			buses[i].bus->Show();
		}	else if(buses[i].busW) {
			buses[i].busW->SetBrightness(brightness);
			buses[i].busW->Show();
		}
	}
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

