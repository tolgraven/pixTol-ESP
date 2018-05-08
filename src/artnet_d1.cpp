#include "artnet_d1.h"

WiFiUDP udp;
ArtnetnodeWifi artnet;
HomieNode pixtolNode("pixtol", "settings");
HomieNode modeNode("dmx", "switch");
HomieNode logNode("log", "level");
// NODES: ConfigNode, StateNode, more?

HomieSetting<bool> cfg_debug("debug", 
    "debug mode, extra logging and pause at start to allow OTA flash even if unit is crashing").setDefaultValue(true);
HomieSetting<long> cfg_log_artnet(   "log_artnet", 			"log level for incoming artnet packets");
HomieSetting<bool> cfg_log_to_serial("log_to_serial", 	"whether to log to serial");
HomieSetting<bool> cfg_log_to_mqtt(  "log_to_mqtt", 		"whether to log to mqtt");
HomieSetting<long> cfg_dmx_hz(       "dmx_hz", 	        "dmx frame rate");      // use to calculate strobe and interpolation stuff
HomieSetting<long> cfg_strobe_hz_min("strobe_hz_min", 	"lowest strobe frequency");
HomieSetting<long> cfg_strobe_hz_max("strobe_hz_max", 	"highest strobe frequency");
HomieSetting<bool> cfg_mirror(		   "mirror_second_half", "whether to mirror strip back onto itself, for controlling folded strips >1 uni as if one");
HomieSetting<bool> cfg_folded_alternating("folded_alternating", "alternating pixels");
HomieSetting<bool> cfg_clear_on_start("clear_on_start",     "clear strip on boot");
HomieSetting<bool> cfg_flip(        "flip",             "flip strip direction");
HomieSetting<bool> cfg_gamma_correct("gamma_correct",   "shitty gamma correction that sucks");
HomieSetting<long> cfg_count(			"led_count", 					"number of LEDs in strip"); // rework for multiple strips
HomieSetting<long> cfg_bytes(			"bytes_per_pixel", 		"3 for RGB, 4 for RGBW");
HomieSetting<long> cfg_universes(	"universes", 					"number of DMX universes");
HomieSetting<long> cfg_start_uni(	"starting_universe", 	"index of first DMX universe used");
HomieSetting<long> cfg_start_addr("starting_address", 	"index of beginning of strip, within starting_universe."); // individual pixel control starts after x function channels offset (for strobing etc)
// HomieSetting<long> cfg_strips(		"strips", 						"number of LED strips being controlled");

// FN_SAVE writes all current (savable) DMX settings to config.json
// then make cues in afterglow or buttons in max/mira to control settings per strip = no config file bs
// should also set up OSC support tho so can go that way = no fiddling with mapping out and translating shitty 8-bit numbers
// but stuff like flip could be both a config thing and performance effect so makes sense. anything else?
// bitcrunch, pixelate (halving resolution + zoom around where is grabbing)

/* INPUT METHODS:
 * DMX: XLR, IP: Artnet/sACN(?), 
 * OPC?
 * OSC/MQTT (settings/scenes)
 *
 * OUTPUTS:
 * DMX: XLR, IP
 * LEDstrips: 24/32 bits,
 * GPIO for other stuff? use a pixtol to mod fog machine to run DMX etc.
 * */


// GLOBALS FROM SETTINGS
uint8_t bytes_per_pixel, start_uni, universes;
uint16_t led_count;
bool mirror, folded, flipped;
uint8_t log_artnet;
uint8_t hzMin, hzMax;

// NEOPIXELBUS
NeoGamma<NeoGammaTableMethod> *colorGamma;
struct busState {
	// NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>       *bus;	// this way we can (re!)init off config values, post boot
  DmaGRB *bus;
	// NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>       *busW;
  DmaGRBW *busW;
	// NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod> *busW2;
  UartGRBW *busW2;
}; busState* buses = new busState[1](); // XXX didnt get Uart mode working, Dma works but iffy with concurrent bitbanging - stick to bang?  +REMEMBER: DMA and UART must ->Begin() after BitBang...

uint8_t last_data[512] = {0};
uint8_t* last_functions = &last_data[-1];  // take care of DMX ch offset...

Ticker timer_inter;
const uint8_t interFrames = 3; // ~40 us/led gives 5 ms for 1 universe of 125 leds. so around 4-5 frames per frame should be alright.
const float blendBaseline = 1.00f - 1.00f / (1 + interFrames); // for now, make dynamic...

bool shutterOpen = true;
float onFraction = 4;
uint16_t onTime, strobePeriod;
uint8_t ch_strobe_last_value = 0;
uint8_t strobeTickerClosed, strobeTickerOpen;

uint8_t brightness;
uint8_t last_brightness = 0;
float attack, rls;

void initHomie() {
  Homie_setFirmware(FW_NAME, FW_VERSION); Homie_setBrand(FW_BRAND);
	Homie.setConfigurationApPassword(FW_NAME);
  
  // Homie.setLedPin(D2, HIGH); // Homie.disableLedFeedback();
  Homie.setGlobalInputHandler(globalInputHandler);
  // Homie.setBroadcastHandler(broadcastHandler);
  // Homie.setSetupFunction(homieSetup).setLoopFunction(homieLoop);
	// Homie.setLoggingPrinter(); //only takes Serial objects. Mod for mqtt?

  Homie.onEvent(onHomieEvent);
	cfg_start_uni.setDefaultValue(1); cfg_start_addr.setDefaultValue(1); cfg_universes.setDefaultValue(1);
  cfg_clear_on_start.setDefaultValue(true); cfg_flip.setDefaultValue(false); cfg_gamma_correct.setDefaultValue(false);
  cfg_mirror.setDefaultValue(false); cfg_folded_alternating.setDefaultValue(false);
  cfg_log_to_mqtt.setDefaultValue(true); cfg_log_to_serial.setDefaultValue(false);
  cfg_log_artnet.setDefaultValue(2); cfg_debug.setDefaultValue(true);
  cfg_strobe_hz_min.setDefaultValue(1); cfg_strobe_hz_max.setDefaultValue(10);  cfg_dmx_hz.setDefaultValue(40);
  cfg_count.setDefaultValue(120); cfg_bytes.setDefaultValue(4);

  // if(!cfg_debug.get()) Homie.disableLogging();
  Homie.setGlobalInputHandler(globalInputHandler);
  pixtolNode.advertise("on").settable();
  modeNode.advertise("on").settable();
	modeNode.advertise("brightness").settable();

	Homie.setup();
}

void initArtnet() {
  artnet.setName(Homie.getConfiguration().name);
  artnet.setNumPorts(universes);
	artnet.setStartingUniverse(start_uni);
	for(uint8_t i=0; i < universes; i++) artnet.enableDMXOutput(i);
  artnet.enableDMXOutput(0);
	artnet.begin();
  initHomie();

	artnet.setArtDmxCallback(onDmxFrame);
  // udp.begin(ARTNET_PORT); // done by artnetnode

void setup() {
	Serial.begin(SERIAL_BAUD);
	while(!Serial);
  randomSeed(analogRead(0));

	// TODO use led_count to calculate aprox possible frame rate, then use interpolation to fade (say dmx 40 fps, we can run 120 = 3) regardless of/in addition to rls<Plug>/fade stuff
  led_count       = cfg_count.get();          if(mirror) led_count *= 2;	// actual leds is twice conf XXX IMPORTANT: heavier dithering when mirroring
	bytes_per_pixel = cfg_bytes.get();          log_artnet = cfg_log_artnet.get();
  mirror          = cfg_mirror.get();         folded     = cfg_folded_alternating.get();  flipped = cfg_flip.get();
  hzMin           = cfg_strobe_hz_min.get();  hzMax      = cfg_strobe_hz_max.get();  // should these be setable through dmx control ch?                                                      
  start_uni       = cfg_start_uni.get();      universes  = cfg_universes.get();

  if(cfg_gamma_correct.get()) colorGamma = new NeoGamma<NeoGammaTableMethod>;
	if(bytes_per_pixel == 3) { buses[0].bus = new DmaGRB(led_count);	// actually RX pin when DMA
	} else if(bytes_per_pixel == 4) {
      if(cfg_clear_on_start.get()) { // why is this not picked up?
        DmaGRBW tempbus(288);
        tempbus.Begin(); tempbus.Show();
        delay(10); // no idea why the .Show() doesn't seem to take without this? somehow doesnt latch or something?
        Homie.getLogger() << "Cleared up to 288 LEDs" << endl;
      }
    // buses[0].busW = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>(led_count);
    // buses[0].busW2 = new NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod>(led_count);
		buses[0].busW  = new DmaGRBW(led_count);
		buses[0].busW2 = new UartGRBW(led_count);
	}
  if(buses[0].bus) {
    buses[0].bus->Begin(); //if(cfg_clear_on_start.get()) buses[0].bus->Show();
  }
  if(buses[0].busW) {
    buses[0].busW->Begin(); //if(cfg_clear_on_start.get()) buses[0].busW->Show();
    buses[0].busW2->Begin(); //if(cfg_clear_on_start.get()) buses[0].busW2->Show();
  }

	initArtnet();

	setupOTA(led_count);
  if(cfg_debug.get()){
    Homie.getLogger() << "Chance to flash OTA before entering main loop";
    for(int8_t i = 0; i < 5; i++) {
      ArduinoOTA.handle(); // give chance to flash new code in case loop is crashing
      delay(100);
      Homie.getLogger() << ".";
    }
  }
  // XXX check wifi status, if not up for long try like, reset config but first set default values of everything to curr values of everything, so no need touch rest. or just stash somewhere on fs, read back and replace wifi. always gotta be able to recover fully from auto soft-reset, in case of just eg router power went out temp...
  // other potential, scan wifi for other pixtol APs. if none avail, create (inc mqtt broker). if one is, connect...
  // scan and quick connect could also mean "lead" esp tells others new AP/pass so only need to config one!!!
  // put it in a setup mode then once setup keep AP  upuntil rest have switched to correct net

	Homie.getLogger() << endl << "Setup completed" << endl << endl;
  // THIS is why startup has been so slow?  "the connection to the MQTT broker is blocking during ~5 seconds in case the server is unreachable. This is an Arduino for ESP8266 limitation, and we can't do anything on our side to solve this issue, not even a timeout."
}

bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value) {
  Homie.getLogger() << "Received on node " << node.getId() << ": " << property << " = " << value << endl;
  if(value=="reset") Homie.reset();
//switch(value) {
//  case "reset": Homie.reset(); break;
//}

  return true;
}

int getPixelIndex(int pixel) { // XXX also handle matrix back-and-forth setups etc
  if(folded) { // pixelidx differs from pixel recieved
    if(pixel % 2) return pixel/2;  // since every other pixel is from opposite end
    else          return led_count-1 - pixel/2;
  } else     return pixel;
}

int getPixelFromIndex(int pixelidx) {

}

uint8_t iteration = 0;
int8_t subPixelNoise[125][4] = {0};

void updatePixels(uint8_t busidx, uint8_t* data) { // XXX also pass fraction in case interpolating >2 frames
  uint8_t* functions = &data[-1];
  int data_size = bytes_per_pixel * cfg_count.get() + DMX_FN_CHS;
	int pixel = 0; //512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
  int pixelidx;
	NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> *busW = buses[busidx].busW;
  float brightnessFraction = 255 / (brightness ? brightness : 1);     // ahah can't believe divide by zero got me

  for(int t = DMX_FN_CHS; t < data_size; t += bytes_per_pixel) {
    pixelidx = getPixelIndex(pixel);
    
		if(bytes_per_pixel == 3) {  // create wrapper possibly so dont need this repeat shit.
      // use ColorObject base class etc...
			RgbColor color 	= RgbColor(data[t], data[t+1], data[t+2]);
      color = RgbColor::LinearBlend(buses[busidx].bus->GetPixelColor(pixelidx), color, attack);
      if(color.CalculateBrightness() < 12) color.Darken(12); // avoid bitcrunch
			buses[busidx].bus->SetPixelColor(pixelidx, color); // TODO apply light dithering if resolution can sustain it, say brightness > 20%
			if(mirror) buses[busidx].bus->SetPixelColor(led_count - pixel - 1, color);


		} else if(bytes_per_pixel == 4) { 
      // NOISE / DITHERING BS
      uint8_t maxNoise = 32; //baseline, 10 either direction
      if(functions[CH_NOISE]) {
        maxNoise = (1 + functions[CH_NOISE]) / 4 + maxNoise; // CH_NOISE at max gives +-48
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

      uint8_t pixelBrightness = color.CalculateBrightness();
      RgbwColor lastColor = busW->GetPixelColor(pixelidx);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
      // XXX need to actually restore lastColor to its original brightness before mixing tho...
      color = RgbwColor::LinearBlend(color, lastColor, (brighter ? attack : rls));
			if(color.CalculateBrightness() < 6) { // avoid bitcrunch
        color.Darken(6); // XXX should rather flag pixel for temporal dithering yeah?
      } else { // HslColor(); //no go from rgbw...
      }

      if(cfg_gamma_correct.get()) color = colorGamma->Correct(color); // test. better response but fucks resolution a fair bit. Also wrecks saturation? and general output. Fuck this shit

      // all below prob go in own func so dont have to think about them and can reuse in next loop...
      if(!flipped) busW->SetPixelColor(pixelidx, color);
      else         busW->SetPixelColor(led_count-1 - pixelidx, color);
			buses[busidx].busW2->SetPixelColor(led_count-1 - pixelidx, color); //test, mirror so can see whether works...

			if(mirror) busW->SetPixelColor(led_count-1 - pixelidx, color);   // XXX offset colors against eachother here! using HSL even, (one more saturated, one lighter).  Should bring a tiny gradient and look nice (compare when folded strip not mirrored and loops back with glorious color combination results)
		} pixel++;
	}

  if(functions[CH_BLEED]) { // loop through again...
    for(pixel = 0; pixel < led_count; pixel++) {
      pixelidx = getPixelIndex(pixel);
			RgbwColor color = busW->GetPixelColor(pixelidx);
      RgbwColor prevPixelColor = color; // same concept just others rub off on it instead of other way...
      RgbwColor nextPixelColor = color;
      // def cant look at data, remember pixelidx hey. I guess make two runs, first setting all pixels then run blend?
      if(pixel - 1) prevPixelColor = busW->GetPixelColor(getPixelIndex(pixel - 1));
      if(pixel + 1 < led_count) nextPixelColor = busW->GetPixelColor(getPixelIndex(pixel + 1));
      float amount = (float)functions[CH_BLEED]/(256*2);  //this way only ever go half way = before starts decreasing again
      // float amount = 0.50f; //test
      color = RgbwColor::BilinearBlend(color, nextPixelColor, prevPixelColor, color, amount, amount);

      busW->SetPixelColor(pixelidx, color); // XXX handle flip and that

    }
  }

}

void updateFunctions(uint8_t busidx, uint8_t* functions, bool isKeyframe) {
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

  // if(functions[CH_ROTATE_FWD && isKeyframe]) { // why the fuck is this working? :O (note fucked brace) guess just returns true and tada
  if(functions[CH_ROTATE_FWD]) {
  // if(functions[CH_ROTATE_FWD] && isKeyframe) {
		if(buses[busidx].bus)        buses[busidx].bus->RotateRight(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
		else if(buses[busidx].busW)  buses[busidx].busW->RotateRight(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
  // } if(isKeyframe && functions[CH_ROTATE_BACK] && (functions[CH_ROTATE_BACK] != last_functions[CH_ROTATE_BACK])) { // so what happens is since this gets called in interpolation it shifts like five extra times
  } if(functions[CH_ROTATE_BACK]) { // very cool kaozzzzz strobish when rotating strip folded, 120 long but defed 60 in afterglow. explore!!
		if(buses[busidx].bus)        buses[busidx].bus->RotateLeft(led_count * ((float)functions[CH_ROTATE_BACK] / 255));
		else if(buses[busidx].busW)  buses[busidx].busW->RotateLeft(led_count * ((float)functions[CH_ROTATE_BACK] / 255));
  }

  switch(functions[CH_CONTROL]) {
    case FN_FRAMEGRAB_1 ... FN_FRAMEGRAB_4: break;
    case FN_FLIP: flipped = !flipped; break;
  }

  //WTF: when not enough current and strips yellow, upping atttack makes them less yellow. Also happens in general, but less noticable?
  if(functions[CH_ATTACK] != last_functions[CH_ATTACK]) {
    attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_ATTACK]/285); // dont ever quite arrive like this, two runs at max = 75% not 100%...
    if(log_artnet >= 2) Homie.getLogger() << "Attack: " << functions[CH_ATTACK] << " / " << attack << endl;
  }
  if(functions[CH_RELEASE] != last_functions[CH_RELEASE])
    rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_RELEASE]/285);

  if(functions[CH_DIMMER_ATTACK] != last_functions[CH_DIMMER_ATTACK])
    dimmer_attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_ATTACK]/285); // dont ever quite arrive like this, two runs at max = 75% not 100%...
  if(functions[CH_DIMMER_RELEASE] != last_functions[CH_DIMMER_RELEASE])
    dimmer_rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_RELEASE]/285);

  bool brighter = brightness > last_brightness;
	brightness = last_brightness + (functions[CH_DIMMER] - last_brightness) * (brighter ? dimmer_attack : dimmer_rls);
	if(brightness < 4) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
  else if(brightness > 255) brightness = 255;
	if(shutterOpen) {
		if(buses[busidx].bus) buses[busidx].bus->SetBrightness(brightness);
		else if(buses[busidx].busW) {
      buses[busidx].busW->SetBrightness(brightness);
      buses[busidx].busW2->SetBrightness(brightness);
    }
	} else buses[busidx].busW->SetBrightness(0);

  last_brightness = brightness;
}

void renderInterFrame() {
  uint8_t busidx = 0;
  updatePixels(busidx, last_data);
  updateFunctions(busidx, last_functions, false); // XXX fix so interpolates!!
  buses[busidx].busW->Show();
  buses[busidx].busW2->Show();
}

uint8_t interCounter;

void interCallback() {
  if(interCounter > 1) { // XXX figure out on its own whether will overflow, so can just push towards max always
    timer_inter.once_ms((1000/interFrames / cfg_dmx_hz.get()), interCallback);
  }
  renderInterFrame(); // should def try proper temporal dithering here...
  interCounter--;
}

// unsigned long renderMicros; // seems just over 10us per pixel, since 125 gave around 1370... much faster than WS??

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
	if(universe < start_uni || universe >= start_uni + universes) return; // need to expand with different subnets etc...
  // if(modeNode.) // is set to dmx control
	if(log_artnet >= 3) Homie.getLogger() << universe;
  // if(log_artnet >= 2) renderMicros = micros();
	uint8_t* functions = &data[-1];  // take care of DMX ch offset...
	uint8_t busidx = universe - start_uni; // XXX again watch out for uni 0...

  // bool changed = false;
  // for(uint16_t i=0; i < 512; i++) {
  //   if(data[i] != last_data[i]) {
  //     changed = true; break; // don't process unless something differs
  // } } if(!changed) return; // decent idea but fucks too much atm, work on other solutions for strobe woes instead...

  updatePixels(busidx, data);
  updateFunctions(busidx, functions, true);

  if(buses[busidx].bus) buses[busidx].bus->Show();
  else if(buses[busidx].busW) {
    buses[busidx].busW->Show();
    buses[busidx].busW2->Show();
  }
  // if(log_artnet >= 2) Homie.getLogger() << micros() - renderMicros << " ";
  memcpy(last_data, data, sizeof(uint8_t) * 512);
  interCounter = interFrames;
  if(interCounter) timer_inter.once_ms((1000/interFrames / cfg_dmx_hz.get()), interCallback);
}

void loopArtNet() {
  switch(artnet.read()) { // check opcode for logging purposes, actual logic in callback function
    case OpDmx:  if(log_artnet >= 3) Homie.getLogger() << "DMX "; 					 break;
    case OpPoll: if(log_artnet >= 1) Homie.getLogger() << "Art Poll Packet"; break;
	}
}

void loop() {
  // Homie.reset
	loopArtNet(); // if this doesn't result in anything it should still call updatePixels a few times to finish fades etc.
  ArduinoOTA.handle();
  yield();
	Homie.loop();
}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::STANDALONE_MODE:
      break;
    case HomieEventType::CONFIGURATION_MODE: // blink orange or something?
      // blinkStrip(led_count, orange, 5);
      break;
    case HomieEventType::NORMAL_MODE:
      // blinkStatus(blue, 3);
      break;
    case HomieEventType::OTA_STARTED:
      // blinkStrip(led_count, yellow, 0);
      break;
    case HomieEventType::OTA_PROGRESS: // can use event.sizeDone and event.sizeTotal
      // buses[0].busW->SetPixelColor(event.sizeDone / (event.sizeTotal / led_count), blue);
      // buses[0].busW->Show();
      break;
    case HomieEventType::OTA_FAILED:
      // blinkStrip(led_count, red, 2);
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      // blinkStrip(led_count, green, 2);
      break;
    case HomieEventType::ABOUT_TO_RESET:
      // blinkStrip(led_count, orange, 0);
      break;
    case HomieEventType::WIFI_CONNECTED: // normal mode, can use event.ip, event.gateway, event.mask
      // blinkStrip(led_count, green, 1);
      break;
    case HomieEventType::WIFI_DISCONNECTED: // normal mode, can use event.wifiReason
      // blinkStrip(led_count, red, 0);
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

