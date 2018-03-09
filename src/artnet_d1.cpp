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

#define DMX_FN_CHS 8
#define CH_DIMMER 1
#define CH_STROBE 2
#define CH_STROBE_CURVES 3
#define CH_ATTACK 4
#define CH_RELEASE 5
#define CH_BLEED 6
#define CH_NOISE 7
#define CH_ROTATE_FWD 8
#define CH_ROTATE_BACK 9

#define CH_FRAMEGRAB 12
// breakdown above: like 0 nil 1-4/8(?) grab cur to slot, 10-255 some nice enough curve driving x+y in a bilinear blend depending on amt of slots

HomieSetting<long> cfg_log_artnet(   "log_artnet", 			"log level for incoming artnet packets");
HomieSetting<bool> cfg_log_to_serial("log_to_serial", 	"whether to log to serial");
HomieSetting<bool> cfg_log_to_mqtt(  "log_to_mqtt", 		"whether to log to mqtt");
HomieSetting<long> cfg_dmx_hz(       "dmx_hz", 	        "dmx frame rate");      // use to calculate strobe and interpolation stuff
HomieSetting<long> cfg_strobe_hz_min("strobe_hz_min", 	"lowest strobe frequency");
HomieSetting<long> cfg_strobe_hz_max("strobe_hz_max", 	"highest strobe frequency");
// HomieSetting<long> cfg_strips(		"strips", 						"number of LED strips being controlled");
HomieSetting<bool> cfg_mirror(		"mirror_second_half", "whether to mirror strip back onto itself, for controlling folded strips >1 uni as if one");
HomieSetting<bool> cfg_folded_alternating("folded_alternating", "alternating pixels");
HomieSetting<bool> cfg_clear_on_start(    "clear_on_start",     "clear strip on boot");
HomieSetting<bool> cfg_flip(        "flip",             "flip strip direction");
HomieSetting<bool> cfg_clear_on_start(    "clear_on_start",     "clear strip on boot");
HomieSetting<bool> cfg_flip(        "flip",             "flip strip direction");
// HomieSetting<long> cfg_pin(				"led_pins", 					"pin to use for LED strip control"); // should be array. D1 = 5 / GPIO05
HomieSetting<long> cfg_count(			"led_count", 					"number of LEDs in strip"); // rework for multiple strips
HomieSetting<long> cfg_bytes(			"bytes_per_pixel", 		"3 for RGB, 4 for RGBW");
HomieSetting<long> cfg_universes(	"universes", 					"number of DMX universes");
HomieSetting<long> cfg_start_uni(	"starting_universe", 	"index of first DMX universe used");
HomieSetting<long> cfg_start_addr("starting_address", 	"index of beginning of strip, within starting_universe."); // individual pixel control starts after x function channels offset (for strobing etc)

uint8_t bytes_per_pixel, start_uni, universes;
uint16_t led_count;
bool mirror, folded;
uint8_t log_artnet;
uint8_t brightness, hzMin, hzMax;
struct busState {
	// bool shutterOpen; // actually do we even want this per bus? obviously for strobe etc this is one unit, no? so one shutter, one timer etc. but then further hax on other end for fixture def larger than one universe...  prob just use first connected strip's function channels, rest are slaved?
	NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod> *bus;	// this way we can (re!)init off config values, post boot
	NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> *busW;	// this way we can (re!)init off config values, post boot
};

WiFiUDP udp;
ArtnetnodeWifi artnet;
// XXX didnt get Uart mode working, Dma works but iffy with concurrent bitbanging - stick to bang?  +REMEMBER: DMA and UART must ->Begin() after BitBang...
busState* buses = new busState[2]();
bool shutterOpen = true;
Ticker timer_strobe_predelay, timer_strobe_each, timer_strobe_on_for;
float onFraction = 5;
uint16_t onTime;
uint8_t ch_strobe_last_value = 0;

void setupOTA();
void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);
void shutterCloseCallback(); //uint8_t idx);
void shutterOpenCallback(); //uint8_t idx);

  cfg_clear_on_start.setDefaultValue(1); cfg_flip.setDefaultValue(0);
void setup() {
	Homie.disableResetTrigger(); Homie_setFirmware("artnet", "1.0.1");
  // cfg_pin.setDefaultValue(LED_PIN); 
	cfg_start_uni.setDefaultValue(1); cfg_start_addr.setDefaultValue(1); cfg_dmx_hz.setDefaultValue(40);

	Serial.begin(115200);
	Homie.setup();
  led_count = cfg_count.get();        if(mirror) led_count *= 2;	// actual leds is twice conf XXX IMPORTANT: heavier dithering when mirroring
	bytes_per_pixel = cfg_bytes.get();  log_artnet = cfg_log_artnet.get();
  mirror = cfg_mirror.get();          folded = cfg_folded_alternating.get();
  hzMin = cfg_strobe_hz_min.get();    hzMax = cfg_strobe_hz_max.get();  // should these be setable through dmx control ch?                                                      
  start_uni = cfg_start_uni.get();    universes = cfg_universes.get();
	// TODO use led_count to calculate aprox possible frame rate, then use interpolation to fade (say dmx 40 fps, we can run 120 = 3) regardless of/in addition to rls<Plug>/fade stuff

	if(bytes_per_pixel == 3) {
		buses[0].bus = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>(led_count, D1);	// actually RX pin when DMA
		// buses[1].bus = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
	} else if(bytes_per_pixel == 4) {
		buses[0].busW = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>(led_count, D1);
		// buses[1].busW = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(led_count, D2);
	}
	for(int i = 0; i < 2; i++) {
		if(buses[i].bus)  buses[i].bus->Begin();
		if(buses[i].busW) buses[i].busW->Begin();
	}

  artnet.setName(Homie.getConfiguration().name);
  artnet.setNumPorts(universes);  artnet.setStartingUniverse(start_uni);
	uint8_t uniidx = start_uni - 1; // starting universe must be > 0
	for(uint8_t i=uniidx; i < uniidx + cfg_universes.get(); i++) artnet.enableDMXOutput(i);
	artnet.enableDMXOutput(0); artnet.enableDMXOutput(1); artnet.enableDMXOutput(2); artnet.enableDMXOutput(3);
	artnet.begin();   artnet.setArtDmxCallback(flushNeoPixelBus);
  udp.begin(ARTNET_PORT);

	setupOTA();

	Homie.getLogger() << endl << "Setup completed" << endl << endl;
}


void loopArtNet() {
  switch(artnet.read()) { // check opcode for logging purposes, actual logic in callback function
    case OpDmx:  if(log_artnet >= 3) Homie.getLogger() << "DMX "; 					 break;
    case OpPoll: if(log_artnet >= 1) Homie.getLogger() << "Art Poll Packet"; break;
	}
}

void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
	if(universe < start_uni || universe >= start_uni + cfg_universes.get()) return; // need to expand with different subnets etc...

	if(log_artnet >= 2) Homie.getLogger() << universe;
	uint8_t* functions = &data[-1];  // take care of DMX ch offset...
	uint16_t pixel = 0; //512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
  uint16_t pixelidx;
  bool even = true;
	uint8_t busidx = universe - start_uni; // XXX again watch out for uni 0...
  float attack = 1.00f - (float)functions[CH_ATTACK]/275; //275 tho max is 255 so can use full range without crapping out...
  float rls = 1.00f - (float)functions[CH_RELEASE]/275;

  for(uint16_t t = DMX_FN_CHS; t < bytes_per_pixel*cfg_count.get()+DMX_FN_CHS; t+=bytes_per_pixel) {
    pixelidx = pixel;
    if(folded) {
      pixelidx = (even ? pixel/2 : led_count-1 - pixel/2);
      even = !even;
    }
		if(bytes_per_pixel == 3) {
			RgbColor color 	= RgbColor(data[t], data[t+1], data[t+2]);
      color = RgbColor::LinearBlend(buses[busidx].bus->GetPixelColor(pixelidx), color, attack);
      if(color.CalculateBrightness() < 16) color.Darken(16); // avoid bitcrunch
			buses[busidx].bus->SetPixelColor(pixelidx, color); // TODO apply light dithering if resolution can sustain it, say brightness > 20%
			if(mirror) buses[busidx].bus->SetPixelColor(led_count - pixel - 1, color); // XXX offset colors against eachother here! using HSL even, (one more saturated, one lighter).
      // Should bring a tiny gradient and look nice (compare when folded strip not mirrored and loops back with glorious color combination results)

		} else if(bytes_per_pixel == 4) { 
			RgbwColor color = RgbwColor(data[t], data[t+1], data[t+2], data[t+3]);
      RgbwColor lastColor = buses[busidx].busW->GetPixelColor(pixelidx);
      bool brighter = color.CalculateBrightness() > lastColor.CalculateBrightness();
      color = RgbwColor::LinearBlend(lastColor, color, (brighter ? attack : rls));
			if(color.CalculateBrightness() < 8) color.Darken(8); // avoid bitcrunch
			buses[busidx].busW->SetPixelColor(pixelidx, color);
			if(mirror) buses[busidx].busW->SetPixelColor(led_count - pixel - 1, color);
		} pixel++;
	}
}

void updateFunctions(uint8_t busidx, uint8_t* functions) {
	if(busidx != 0) return; // first strip is master (at least for strobe etc)
  if(functions[CH_STROBE]) {
    if(functions[CH_STROBE] != ch_strobe_last_value) { // reset timer for new state
      // XXX does same stupid thing as ADJ wash when sweeping strobe rate. because it starts instantly I guess. throttle somehow, or offset actual strobe slightly.
      float hz = ((hzMax - hzMin) * (functions[CH_STROBE] - 1) / (255 - 1)) + hzMin;
      strobePeriod = 1000 / hz;   // 1 = 1 hz, 255 = 10 hz, to test
      // if(functions[CH_STROBE_CURVES]) { //use non-default fraction
      // 	// buses[busidx].onFraction = ...
      // }
      onTime = strobePeriod / onFraction; // arbitrary default val. Use as midway point to for period control >127 goes up, < down

      // timer_strobe_predelay.once_ms(8, shutterOpenCallback); // 1 frame at 40hz = 25 ms so prob dont want to go above  10 before double flush
      // timer_strobe_each.attach_ms(strobePeriod, shutterOpenCallback); //, busidx);
      timer_strobe_each.once_ms(strobePeriod, shutterOpenCallback); //, busidx);
      // shutterOpen = false;
    }
  } else { // 0, clean up
    timer_strobe_on_for.detach();
    timer_strobe_each.detach();
    shutterOpen = true;
  }
  ch_strobe_last_value = functions[CH_STROBE];

  if(functions[CH_ROTATE_BACK]) { // this is fucked now esp with high release, seperates blob somehow (not a delay thing, stays like that). BUT looks really cool in a way so even if fix, keep curr behavior available...
		if(buses[busidx].bus)        buses[busidx].bus->RotateLeft(functions[CH_ROTATE_BACK]);
		else if(buses[busidx].busW)  buses[busidx].busW->RotateLeft(functions[CH_ROTATE_BACK]);
  } else if(functions[CH_ROTATE_FWD]) {
		if(buses[busidx].bus)        buses[busidx].bus->RotateRight(functions[CH_ROTATE_FWD]);
		else if(buses[busidx].busW)  buses[busidx].busW->RotateRight(functions[CH_ROTATE_FWD]);
  }
  switch(functions[CH_CONTROL]) {
    case FN_FRAMEGRAB: break;
void interpolatePixelsCallback() {
  uint8_t busidx = 0;
  updatePixels(busidx, last_data);
  updateFunctions(busidx, last_functions); // XXX fix so interpolates!!
  buses[busidx].busW->Show();
}


void flushNeoPixelBus(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
	if(universe < start_uni || universe >= start_uni + universes) return; // need to expand with different subnets etc...
  bool changed = false;
  for(uint16_t i=0; i < 512; i++) {
    if(data[i] != last_data[i]) {
      changed = true; break; // don't process unless something differs
  } }
  // if(!changed) return; // decent idea but fucks too much atm, work on other solutions for strobe woes instead...

	if(log_artnet >= 3) Homie.getLogger() << universe;
	uint8_t* functions = &data[-1];  // take care of DMX ch offset...
	uint8_t busidx = universe - start_uni; // XXX again watch out for uni 0...
  rls = blendBaseline + blendBaseline * ((float)functions[CH_RELEASE]/285);
  if(functions[CH_ATTACK] != last_data[CH_ATTACK - 1]) {
    attack = blendBaseline + blendBaseline * ((float)functions[CH_ATTACK]/285); // dont ever arrive like this, two runs at max = 75% not 100%...
    if(log_artnet >= 2) Homie.getLogger() << "Attack: " << functions[CH_ATTACK] << " / " << attack << endl;
  }

  float dimmer_attack = blendBaseline - (float)functions[CH_DIMMER_ATTACK]/275; //275 tho max is 255 so can use full range without crapping out...
  float dimmer_rls = blendBaseline - (float)functions[CH_DIMMER_RELEASE]/275;

  updatePixels(busidx, data);
  updateFunctions(busidx, functions);

  // last_data = data;
  memcpy(last_data, data, sizeof(uint8_t) * 512);
  timer_inter_pixels.once_ms((1000/2) / cfg_dmx_hz.get(), interpolatePixelsCallback);
}

void loopArtNet() {
  switch(artnet.read()) { // check opcode for logging purposes, actual logic in callback function
    case OpDmx:  if(log_artnet >= 3) Homie.getLogger() << "DMX "; 					 break;
    case OpPoll: if(log_artnet >= 1) Homie.getLogger() << "Art Poll Packet"; break;
	}
}

    case FN_FLIP: break;
  }
	brightness = last_brightness + (functions[CH_DIMMER] - last_brightness) * blendBaseline;
  // then interpolate based on last brightness, dimmer_attack/rls
  // should also get between-frame interpolation!!
	if(brightness < 8) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
  last_brightness = brightness;
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

