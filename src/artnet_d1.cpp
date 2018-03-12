#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <Homie.h>
#include <WiFiUdp.h>
#include <ArtnetnodeWifi.h>
#include <NeoPixelBrightnessBus.h>
#include <Ticker.h>

// D1=5, D2=4, D3=0, D4=2  D0=16, D55=14, D6=12, D7=13, D8=15
#define LED_PIN D1
#define RX_PIN 3
#define ARTNET_PORT 6454

#define DMX_FN_CHS 12
#define CH_DIMMER 1
#define CH_STROBE 2
#define CH_STROBE_CURVES 3
#define CH_ATTACK 4
#define CH_RELEASE 5
#define CH_BLEED 6
#define CH_NOISE 7
#define CH_ROTATE_FWD 8
#define CH_DIMMER_ATTACK 10
#define CH_DIMMER_RELEASE 11
#define CH_CONTROL 12
#define CH_ROTATE_BACK 9

#define FN_FRAMEGRAB 200
#define FN_FLIP 1
#define FN_SAVE 255
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
uint8_t last_brightness = 0;
float attack, rls;
Ticker timer_inter_pixels;

uint8_t bytes_per_pixel, start_uni, universes;
NeoGamma<NeoGammaTableMethod> colorGamma;
uint8_t last_data[512] = {0};
uint8_t* last_functions = &last_data[-1];  // take care of DMX ch offset...
RgbwColor black = RgbwColor(0, 0, 0, 0);
float blendBaseline = 1.00f / 2; // for now, make dynamic...
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
float onFraction = 4;
uint16_t onTime, strobePeriod;
uint8_t ch_strobe_last_value = 0;
uint8_t strobeTickerClosed, strobeTickerOpen;

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
      // NOISE / DITHERING BS
      uint8_t maxNoise = 24; // test, actually get from fn ch
      if(functions[CH_NOISE]) {
        maxNoise = functions[CH_NOISE] / 4 + maxNoise;
      }
      uint8_t subPixel[4];
      for(uint8_t i=0; i < 4; i++) {
        if(iteration == 1) subPixelNoise[pixelidx][i] = random(maxNoise) - maxNoise/2;
        else if(iteration >= interFrames * 10) iteration = 0;
        iteration++;

        subPixel[i] = data[t+i];
        if(subPixel[i] > 16) {
          int16_t tot = subPixel[i] + subPixelNoise[pixelidx][i];
          if(tot >= 0 && tot <= 255) subPixel[i] = tot;
          else if(tot < 0) subPixel[i] = 0;
          else subPixel[i] = 255;
        }
      }

			RgbwColor color = RgbwColor(subPixel[0], subPixel[1], subPixel[2], subPixel[3]);

      if(functions[CH_BLEED]) {
        RgbwColor prev = color; // same concept just others rub off on it instead of other way...
        RgbwColor next = color;
        if(t - 4 >= DMX_FN_CHS) prev = RgbwColor(data[t-4], data[t-3], data[t-2], data[t-1]); // could grab last pixelidx but then weight not even vs next hmm
        if(t + 7 < data_size) next = RgbwColor(data[t+4], data[t+5], data[t+6], data[t+7]);
        // float amount = (float)functions[CH_BLEED]/(256*2);  //this way only ever go half way = before starts decreasing again
        float amount = 0.20f; //test
        color = RgbwColor::BilinearBlend(color, next, prev, color, amount, amount);
      }

      uint8_t pixelBrightness = color.CalculateBrightness();
      RgbwColor lastColor = buses[busidx].busW->GetPixelColor(pixelidx);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (255/brightness) * lastColor.CalculateBrightness(); // handle any offset from lowering dimmer
      color = RgbwColor::LinearBlend(color, lastColor, (brighter ? attack : rls));
			if(color.CalculateBrightness() < 4) { color.Darken(4); // avoid bitcrunch
      } else { // HslColor(); //no go from rgbw...
      }
      // color = colorGamma.Correct(color); // test. better response but fucks resolution a fair bit. Also wrecks saturation? and general output. Fuck this shit
			buses[busidx].busW->SetPixelColor(pixelidx, color);
			if(mirror) buses[busidx].busW->SetPixelColor(led_count - pixel - 1, color);
		} pixel++;
	}
}

void updateFunctions(uint8_t busidx, uint8_t* functions) {
	if(busidx != 0) return; // first strip is master (at least for strobe etc)

  if(functions[CH_STROBE]) {
    if(functions[CH_STROBE] != ch_strobe_last_value) { // reset timer for new state
      float hz = ((hzMax - hzMin) * (functions[CH_STROBE] - 1) / (255 - 1)) + hzMin;
      strobePeriod = 1000 / hz;   // 1 = 1 hz, 255 = 10 hz, to test
      // if(functions[CH_STROBE_CURVES]) { //use non-default fraction
      // }
      onTime = strobePeriod / onFraction; // arbitrary default val. Use as midway point to for period control >127 goes up, < down
      // XXX instead of timers, use counter and strobe on frames? even 10hz would just be 1 on 3 off, easy...  more precise then use inter frames, yeah?
      strobeTickerClosed = interFrames * strobePeriod / cfg_dmx_hz.get(); // take heed of interframes...
      strobeTickerOpen = interFrames * onTime / cfg_dmx_hz.get();
      shutterOpen = false;
    } else { // decr tickers
      if(shutterOpen) strobeTickerOpen--;
      else            strobeTickerClosed--;
      if(!strobeTickerClosed) {
        shutterOpen = true;
        strobeTickerClosed = strobePeriod / cfg_dmx_hz.get();
      } else if(!strobeTickerOpen) {
        shutterOpen = false;
        strobeTickerOpen = onTime / cfg_dmx_hz.get();
      }
    }
  } else { // 0, clean up
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
    case FN_FRAMEGRAB_1 ... FN_FRAMEGRAB_4: break;
    case FN_FLIP: break;
  }
  float dimmer_attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_ATTACK]/285); // dont ever arrive like this, two runs at max = 75% not 100%...
  float dimmer_release = blendBaseline + (1.00f - blendBaseline) * (float)functions[CH_DIMMER_RELEASE]/285;
  bool brighter = brightness > last_brightness;
	brightness = last_brightness + (functions[CH_DIMMER] - last_brightness) * (brighter ? attack : rls);
	if(brightness < 4) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
	if(shutterOpen) {
		if(buses[busidx].bus) buses[busidx].bus->SetBrightness(brightness);
		else if(buses[busidx].busW) {
      buses[busidx].busW->SetBrightness(brightness);
      buses[busidx].busW2->SetBrightness(brightness);
    }
	} else buses[busidx].busW->SetBrightness(0);

  last_brightness = brightness;
}

void interpolatePixelsCallback() {
  uint8_t busidx = 0;
  updatePixels(busidx, last_data);
  updateFunctions(busidx, last_functions); // XXX fix so interpolates!!
  buses[busidx].busW->Show();
  buses[busidx].busW2->Show();

	// if(log_artnet >= 2) Homie.getLogger() << "i ";
}

uint8_t interCounter;

void interpolateCallback() {
  if(interCounter > 1) { // XXX figure out on its own whether will overflow, so can just push towards max always
    timer_inter_pixels.once_ms((1000/interFrames / cfg_dmx_hz.get()), interpolateCallback);
  }
  interpolatePixelsCallback(); // should def try proper temporal dithering here...
  interCounter--;
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
  if(functions[CH_ATTACK] != last_functions[CH_ATTACK]) {
    attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_ATTACK]/285); // dont ever arrive like this, two runs at max = 75% not 100%...
    if(log_artnet >= 2) Homie.getLogger() << "Attack: " << functions[CH_ATTACK] << " / " << attack << endl;
  }
  if(functions[CH_RELEASE] != last_functions[CH_RELEASE]) {
    rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_RELEASE]/285);
    if(log_artnet >= 2) Homie.getLogger() << "Release: " << functions[CH_RELEASE] << " / " << rls << endl;
  }

  updatePixels(busidx, data);
  updateFunctions(busidx, functions);

  if(buses[busidx].bus) buses[busidx].bus->Show();
  else if(buses[busidx].busW) {
    buses[busidx].busW->Show();
    buses[busidx].busW2->Show();
  }
  // if(log_artnet >= 2) Homie.getLogger() << micros() - renderMicros << " ";
  memcpy(last_data, data, sizeof(uint8_t) * 512);
  interCounter = interFrames;
  if(interCounter) timer_inter_pixels.once_ms((1000/interFrames / cfg_dmx_hz.get()), interpolateCallback);
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


void setupOTA() {
  ArduinoOTA.setHostname(Homie.getConfiguration().name);
  ArduinoOTA.onProgress([](unsigned int p, unsigned int tot) {
      // pinMode(RX_PIN, OUTPUT); // try to make a nice lil LED progress bar heheh
      buses[0].busW->SetPixelColor(p / (tot / led_count), blue);
      buses[0].busW->Show();
      Serial.printf("OTA updating - %u%%\r", p / (tot/100));
      });
  ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA Error[%u]: ", error);
      switch(error) { 
        case OTA_AUTH_ERROR:		Serial.println("Auth Failed"); 		break;
        case OTA_BEGIN_ERROR:		Serial.println("Begin Failed");		break;
        case OTA_CONNECT_ERROR:	Serial.println("Connect Failed");	break;
        case OTA_RECEIVE_ERROR:	Serial.println("Receive Failed");	break;
        case OTA_END_ERROR:			Serial.println("End Failed");			break;
	}	}); ArduinoOTA.begin();
}

void blinkStrip(uint8_t busidx, RgbwColor color, uint8_t blinks) {
  if(!buses[busidx].busW) return;
  uint8_t counter = blinks;
  for(uint8_t b=0; b < blinks; b++) {
    for(uint8_t i=0; i < led_count; i++) {
      buses[busidx].busW->SetPixelColor(i, color);
    }
    buses[busidx].busW->Show();
    if(!--counter) return;
    delay(100);
    for(uint8_t i=0; i < led_count; i++) {
      buses[busidx].busW->SetPixelColor(i, color);
    }
    buses[busidx].busW->Show();
  }

}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::STANDALONE_MODE:
      break;
    case HomieEventType::CONFIGURATION_MODE: // blink orange or something?
      blinkStrip(0, orange, 1);
      break;
    case HomieEventType::NORMAL_MODE:
      blinkStrip(0, blue, 1);
      break;
    case HomieEventType::OTA_STARTED:
      blinkStrip(0, yellow, 1);
      break;
    case HomieEventType::OTA_PROGRESS: // can use event.sizeDone and event.sizeTotal
      buses[0].busW->SetPixelColor(event.sizeDone / (event.sizeTotal / led_count), blue);
      buses[0].busW->Show();
      break;
    case HomieEventType::OTA_FAILED:
      blinkStrip(0, red, 3);
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      blinkStrip(0, green, 2);
      break;
    case HomieEventType::ABOUT_TO_RESET:
      blinkStrip(0, yellow, 1);
      // blinkStrip(0, orange, 1);
      break;
    case HomieEventType::WIFI_CONNECTED: // normal mode, can use event.ip, event.gateway, event.mask
      break;
    case HomieEventType::WIFI_DISCONNECTED: // normal mode, can use event.wifiReason
      // blinkStrip(0, red, 1);
      blinkStrip(0, orange, 1);
      // blinkStrip(0, black, 1);
      // blinkStrip(0, yellow, 1);
      // blinkStrip(0, red, 1);
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
