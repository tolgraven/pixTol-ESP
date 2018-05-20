#include "artnet_d1.h"

ConfigNode* cfg;
BatteryNode* battery;

// NODES: ConfigNode, StateNode, more?
// also encapsulate renderer, input, output

/* INPUT METHODS:
 * DMX: XLR, IP: Artnet/sACN(?), 
 * OPC?
 * OSC/MQTT (settings/scenes)
 *
 * OUTPUTS:
 * DMX: XLR, IP
 * LEDstrips: 8/24/32+ bits,
 * Lights: Milight, etcetc
 * GPIO for other stuff? use a pixtol to mod fog machine to run DMX etc.
 * */


// GLOBALS FROM SETTINGS
uint8_t bytes_per_pixel, start_uni, universes;
uint16_t led_count;
bool mirror, folded, flipped;
uint8_t log_artnet;
bool debug;
uint8_t hzMin, hzMax;
bool gammaCorrect;

// NEOPIXELBUS
NeoGamma<NeoGammaTableMethod> *colorGamma;
struct busState {
	DmaGRB *bus;
  DmaGRBW *busW;
  // UartGRBW *busW2; // NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod> *busW2;
}; busState* buses = new busState[1](); // XXX didnt get Uart mode working, Dma works but iffy with concurrent bitbanging - stick to bang?  +REMEMBER: DMA and UART must ->Begin() after BitBang...

uint8_t last_data[512] = {0};
uint8_t* last_functions = &last_data[-1];  // take care of DMX ch offset...

Ticker timer_inter;
uint8_t interFrames; // ~40 us/led gives 5 ms for 1 universe of 125 leds. mirrored 144 rgb a 30 is 9 so 1-2 inters
// so around 4-5 frames per frame should be alright.  XXX NOT if mirrored remember! make dynamic!!!
float blendBaseline;  // float blendBaseline = 1.00f - 1.00f / (1 + interFrames); // for now, make dynamic...

bool shutterOpen = true;
float onFraction = 4;
uint16_t onTime, strobePeriod;
uint8_t ch_strobe_last_value = 0;
uint8_t strobeTickerClosed, strobeTickerOpen;

uint8_t brightness;
uint8_t last_brightness = 0;
uint8_t brightnessOverride = 255;
uint8_t outBrightness = 255;
float attack, rls, dimmer_attack, dimmer_rls;

BitBangGRB statusRing(D4, 12);
uint8_t statusBrightness = 15;


bool brightnessHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("brightness").send(value);
  brightnessOverride = value.toInt();
  if(buses[0].bus) {
      buses[0].bus->SetBrightness(brightnessOverride);
      buses[0].bus->Show();
  }
  if(buses[0].busW) {
      buses[0].busW->SetBrightness(brightnessOverride);
      buses[0].busW->Show();
  }
  // logNode.setProperty("log").send("BRIGHTNESS SET FFS");
  return true;
}
bool colorHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("color").send(value);
  if(value.equals("blue")) {
    if(buses[0].bus) {
        buses[0].bus->ClearTo(blueZ);
        buses[0].bus->Show();
    }
    if(buses[0].busW) {
        buses[0].busW->ClearTo(blue);
        buses[0].busW->Show();
    }
  }
  return true;
}

=======
bool onHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("on").send(value);
  Homie.reset();
  return true;
}

void initHomie() {
  Homie_setFirmware(FW_NAME, FW_VERSION); Homie_setBrand(FW_BRAND);
	Homie.setConfigurationApPassword(FW_NAME);
  
  // Homie.setLedPin(D2, HIGH);
 // Homie.disableLedFeedback();
  Homie.onEvent(onHomieEvent);
  Homie.setGlobalInputHandler(globalInputHandler);
  Homie.setBroadcastHandler(broadcastHandler);
  // Homie.setSetupFunction(homieSetup).setLoopFunction(homieLoop);
	// Homie.setLoggingPrinter(); //only takes Serial objects. Mod for mqtt?

  cfg = new ConfigNode();

  // if(!cfg_debug.get()) Homie.disableLogging();
  modeNode.advertise("on").settable(onHandler);
	modeNode.advertise("brightness").settable(brightnessHandler);
	modeNode.advertise("color").settable(colorHandler);

  battery = new BatteryNode();

	Homie.setup();
}

void initArtnet() {
  artnet.setName(Homie.getConfiguration().name);
  artnet.setNumPorts(cfg->universes.get());
	artnet.setStartingUniverse(cfg->startUni.get());
	for(uint8_t i=0; i < cfg->universes.get(); i++) artnet.enableDMXOutput(i);
  artnet.enableDMXOutput(0);
	artnet.begin();
	artnet.setArtDmxCallback(onDmxFrame);
  // udp.begin(ARTNET_PORT); // done by artnetnode // yup shouldnt need
}

void setup() {
	Serial.begin(SERIAL_BAUD);
	while(!Serial);
  randomSeed(analogRead(0));

  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  initHomie();
  interFrames = cfg->interFrames.get();
  blendBaseline = 1.00f - 1.00f / (1 + interFrames);

  statusRing.Begin();
  statusRing.ClearTo(RgbColor(0, 0, 0));
  statusRing.Show();
  delay(100);
  statusRing.ClearTo(blueZ);
  statusRing.Show();
  delay(10);


  mirror    = cfg->setMirrored.get(); //strip.setMirrored.get();
  folded = cfg->setFolded.get(); // folded     = strip.setFolded.get();
  flipped = false; //cfg->setFlipped.get(); // flipped = strip.setFlipped.get();
  gammaCorrect = false; // strip.setGammaCorrect.get();

  log_artnet = cfg->logArtnet.get();
  debug = true; //cfg->debug.get();

  led_count = cfg->ledCount.get(); // TODO use led_count to calculate aprox possible frame rate
  source_led_count = led_count; //cfg->sourceLedCount.get();
  if(mirror) led_count *= 2;	// actual leds is twice conf XXX IMPORTANT: heavier dithering when mirroring
	bytes_per_pixel = cfg->bytesPerLed.get();

  hzMin = 1; //cfg->strobeHzMin.get();
  hzMax = 10; //cfg->strobeHzMax.get();  // should these be setable through dmx control ch?                                                      
  start_uni = cfg->startUni.get();
  universes = 1; //cfg->universes.get();


  if(cfg->clearOnStart.get()) {
    DmaGRBW tempbus(144);
    tempbus.Begin();
    tempbus.Show();
    delay(50); // no idea why the .Show() doesn't seem to take without this? somehow doesnt latch or something?
    Homie.getLogger() << "Cleared >1 universe of LEDs" << endl;
  }
	if(bytes_per_pixel == 3) {
    buses[0].bus = new DmaGRB(led_count);
    buses[0].bus->Begin();
	} else if(bytes_per_pixel == 4) {
		buses[0].busW  = new DmaGRBW(led_count);
    buses[0].busW->Begin();
	}

	initArtnet(Homie.getConfiguration().name, universes, cfg->startUni.get(), onDmxFrame);
	setupOTA(led_count);

  if(debug) {
    Homie.getLogger() << "Chance to flash OTA before entering main loop";
    for(int8_t i = 0; i < 5; i++) {
      ArduinoOTA.handle(); // give chance to flash new code in case loop is crashing
      delay(100);
      Homie.getLogger() << ".";
    }
	  // LN.setLoglevel(LoggerNode::DEBUG); //default?
  }
  // XXX check wifi status, if not up for long try like, reset config but first set default values of everything to curr values of everything, so no need touch rest. or just stash somewhere on fs, read back and replace wifi. always gotta be able to recover fully from auto soft-reset, in case of just eg router power went out temp...
  // other potential, scan wifi for other pixtol APs. if none avail, create (inc mqtt broker). if one is, connect...
  // scan and quick connect could also mean "lead" esp tells others new AP/pass so only need to config one!!!
  // put it in a setup mode then once setup keep AP  upuntil rest have switched to correct net

	Homie.getLogger() << endl << "Setup completed" << endl << endl;
}

int getPixelIndex(int pixel) { // XXX also handle matrix back-and-forth setups etc
  if(folded) { // pixelidx differs from pixel recieved
    if(pixel % 2) return pixel/2;  // since every other pixel is from opposite end
    else          return led_count-1 - pixel/2;
  } else     return pixel;
}

void updatePixels(uint8_t* data) { // XXX also pass fraction in case interpolating >2 frames
  static uint8_t iteration = 0;
  static int8_t subPixelNoise[125][4] = {0};

  uint8_t* functions = &data[-1];
  // int data_size = bytes_per_pixel * cfg->sourceLedCount.get() + DMX_FN_CHS; // TODO use led_count to calculate aprox possible frame rate
  int data_size = bytes_per_pixel * source_led_count + DMX_FN_CHS; // TODO use led_count to calculate aprox possible frame rate
  // cant do the up/downsampling earlier since need to mix colors and stuff...
	int pixel = 0; //512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
  int pixelidx;
	DmaGRBW *busW = buses[0].busW;
	DmaGRB *bus = buses[0].bus;
  float brightnessFraction = 255 / (brightness ? brightness : 1);     // ahah can't believe divide by zero got me

  for(int t = DMX_FN_CHS; t < data_size; t += bytes_per_pixel) {
    pixelidx = getPixelIndex(pixel);
    
		if(bytes_per_pixel == 3) {  // create wrapper possibly so dont need this repeat shit.  use ColorObject base class etc...
      // uint8_t maxNoise = 32; //baseline, 10 either direction
      // if(functions[CH_NOISE]) maxNoise = (1 + functions[CH_NOISE]) / 4 + maxNoise; // CH_NOISE at max gives +-48

      uint8_t subPixel[3];
      for(uint8_t i=0; i < 3; i++) {
        // if(iteration == 1) subPixelNoise[pixelidx][i] = random(maxNoise) - maxNoise/2;
        // else if(iteration >= interFrames * 10) iteration = 0;
        // iteration++;

        subPixel[i] = data[t+i];
        if(subPixel[i] > 16) {
          int16_t tot = subPixel[i] + subPixelNoise[pixelidx][i];
          if(tot >= 0 && tot <= 255) subPixel[i] = tot;
          else if(tot < 0) subPixel[i] = 0;
          else subPixel[i] = 255;
        }
      }
			RgbColor color = RgbColor(subPixel[0], subPixel[1], subPixel[2]);
      RgbColor lastColor = bus->GetPixelColor(pixelidx);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
      color = RgbColor::LinearBlend(color, lastColor, (brighter ? attack : rls));
			if(color.CalculateBrightness() < 6) { // avoid bitcrunch
        color.Darken(6); // XXX should rather flag pixel for temporal dithering yeah?
      }

      // if(gammaCorrect) color = colorGamma->Correct(color); // test. better response but fucks resolution a fair bit. Also wrecks saturation? and general output. Fuck this shit
      if(!flipped) bus->SetPixelColor(pixelidx, color);
      else         bus->SetPixelColor(led_count-1 - pixelidx, color);

			if(mirror) bus->SetPixelColor(led_count-1 - pixelidx, color);   // XXX offset colors against eachother here! using HSL even, (one more saturated, one lighter).  Should bring a tiny gradient and look nice (compare when folded strip not mirrored and loops back with glorious color combination results)

		} else if(bytes_per_pixel == 4) { 
      uint8_t maxNoise = 32; //baseline, 10 either direction
      if(functions[CH_NOISE]) maxNoise = (1 + functions[CH_NOISE]) / 4 + maxNoise; // CH_NOISE at max gives +-48

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
					// subPixel[i] = (tot < 0? 0: (tot > 255? 255: tot));
        }
      }
			RgbwColor color = RgbwColor(subPixel[0], subPixel[1], subPixel[2], subPixel[3]);
      RgbwColor lastColor = busW->GetPixelColor(pixelidx);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
      // XXX need to actually restore lastColor to its original brightness before mixing tho...
      color = RgbwColor::LinearBlend(color, lastColor, (brighter ? attack : rls));
			if(color.CalculateBrightness() < 6) { // avoid bitcrunch
        color.Darken(6); // XXX should rather flag pixel for temporal dithering yeah?
      }

      if(gammaCorrect) color = colorGamma->Correct(color); // test. better response but fucks resolution a fair bit. Also wrecks saturation? and general output. Fuck this shit

      // all below prob go in own func so dont have to think about them and can reuse in next loop...
      if(!flipped) busW->SetPixelColor(pixelidx, color);
      else         busW->SetPixelColor(led_count-1 - pixelidx, color);

			if(mirror) busW->SetPixelColor(led_count-1 - pixelidx, color);   // XXX offset colors against eachother here! using HSL even, (one more saturated, one lighter).  Should bring a tiny gradient and look nice (compare when folded strip not mirrored and loops back with glorious color combination results)
		} pixel++;
	}

  if(functions[CH_BLEED]) { // loop through again...
    if(bytes_per_pixel == 3) return; // XXX would crash below when RGB and no busW...

    for(pixel = 0; pixel < led_count; pixel++) {
			RgbwColor color = busW->GetPixelColor(getPixelIndex(pixel));
			uint8_t thisBrightness = color.CalculateBrightness();
      RgbwColor prevPixelColor = color; // same concept just others rub off on it instead of other way...
      RgbwColor nextPixelColor = color;
			float weightPrev;
			float weightNext;
      if(pixel-1 >= 0) {
				prevPixelColor = busW->GetPixelColor(getPixelIndex(pixel-1));
				weightPrev = prevPixelColor.CalculateBrightness() / thisBrightness+1;
			}
      if(pixel+1 < led_count) {
				nextPixelColor = busW->GetPixelColor(getPixelIndex(pixel+1));
				weightNext = nextPixelColor.CalculateBrightness() / thisBrightness+1;
				// if(nextPixelColor.CalculateBrightness() < thisBrightness * 0.7) // skip if dark, test
				// 	nextPixelColor = color;
					// do some more shit tho. Important thing is mixing colors, so maybe look at saturation as well as brightness dunno
					// since we have X and Y should weight towards more interesting.
			}
      float amount = (float)functions[CH_BLEED]/(256*2);  //this way only ever go half way = before starts decreasing again
      color = RgbwColor::BilinearBlend(color, nextPixelColor, prevPixelColor, color, amount, amount);

      busW->SetPixelColor(getPixelIndex(pixel), color); // XXX handle flip and that

    }
  }
	if(functions[CH_HUE]) { // rotate hue around, simple. global desat as well?

	}
}

void updateFunctions(uint8_t* functions, bool isKeyframe) {

  if(functions[CH_STROBE]) {
    if(functions[CH_STROBE] != ch_strobe_last_value) { // reset timer for new state
      float hz = ((hzMax - hzMin) * (functions[CH_STROBE] - 1) / (255 - 1)) + hzMin;
      strobePeriod = 1000 / hz;   // 1 = 1 hz, 255 = 10 hz, to test
      onTime = strobePeriod / onFraction; // arbitrary default val. Use as midway point to for period control >127 goes up, < down
      // XXX instead of timers, use counter and strobe on frames? even 10hz would just be 1 on 3 off, easy...  more precise then use inter frames, yeah?
      strobeTickerClosed = interFrames * strobePeriod / cfg->dmxHz.get(); // take heed of interframes...
      strobeTickerOpen = interFrames * onTime / cfg->dmxHz.get();
      shutterOpen = false;
    } else { // decr tickers
      if(shutterOpen) strobeTickerOpen--;
      else            strobeTickerClosed--;
      if(!strobeTickerClosed) {
        shutterOpen = true;
        strobeTickerClosed = interFrames * strobePeriod / cfg->dmxHz.get();
      } else if(!strobeTickerOpen) {
        shutterOpen = false;
        strobeTickerOpen = interFrames * onTime / cfg->dmxHz.get();
      }
    }
  } else { // 0, clean up
    shutterOpen = true;
  }

  ch_strobe_last_value = functions[CH_STROBE];

  if(functions[CH_ROTATE_FWD]) { // if(functions[CH_ROTATE_FWD] && isKeyframe) {
		if(buses[0].bus)        buses[0].bus->RotateRight(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
		else if(buses[0].busW)  buses[0].busW->RotateRight(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
  // } if(isKeyframe && functions[CH_ROTATE_BACK] && (functions[CH_ROTATE_BACK] != last_functions[CH_ROTATE_BACK])) { // so what happens is since this gets called in interpolation it shifts like five extra times
  } if(functions[CH_ROTATE_BACK]) { // very cool kaozzzzz strobish when rotating strip folded, 120 long but defed 60 in afterglow. explore!!
		if(buses[0].bus)        buses[0].bus->RotateLeft(led_count * ((float)functions[CH_ROTATE_BACK] / 255));
	else if(buses[0].busW)  buses[0].busW->RotateLeft(led_count * ((float)functions[CH_ROTATE_BACK] / 255));
  }

  // switch(functions[CH_CONTROL]) {
  //   case FN_FRAMEGRAB_1 ... FN_FRAMEGRAB_4: break;
  //   case FN_FLIP: flipped = !flipped; break;
  // }

  //WTF: when not enough current and strips yellow, upping atttack makes them less yellow. Also happens in general, but less noticable?
  if(functions[CH_ATTACK] != last_functions[CH_ATTACK]) {
    attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_ATTACK]/285); // dont ever quite arrive like this, two runs at max = 75% not 100%...
    if(log_artnet >= 2) Homie.getLogger() << "Attack: " << functions[CH_ATTACK] << " / " << attack << endl;
    LN.logf("updateFunctions", LoggerNode::DEBUG, "Attack: %d", functions[CH_ATTACK]);
  }
  if(functions[CH_RELEASE] != last_functions[CH_RELEASE]) {
      rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_RELEASE]/285);
  }

  if(functions[CH_DIMMER_ATTACK] != last_functions[CH_DIMMER_ATTACK]) {
      dimmer_attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_ATTACK]/285); // dont ever quite arrive like this, two runs at max = 75% not 100%...
  }
  if(functions[CH_DIMMER_RELEASE] != last_functions[CH_DIMMER_RELEASE]) {
      dimmer_rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_RELEASE]/285);
  }

  if(brightnessOverride <= 0) {
    LN.logf("brightness", LoggerNode::INFO, "%s", "no override");
    bool brighter = brightness > last_brightness;
    brightness = last_brightness + (functions[CH_DIMMER] - last_brightness) * (brighter ? dimmer_attack : dimmer_rls);
    // if(brightness < 4) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
    // else if(brightness > 255) brightness = 255;

  } else {
    brightness = brightnessOverride;
  }
  last_brightness = brightness;
  outBrightness = (shutterOpen? brightness: 0);

  if(buses[0].bus) buses[0].bus->SetBrightness(outBrightness);
  else if(buses[0].busW) buses[0].busW->SetBrightness(outBrightness);
}

void renderInterFrame() {
  updatePixels(last_data);
  updateFunctions(last_functions, false); // XXX fix so interpolates!!
  if(buses[0].bus) {
      if(buses[0].bus->CanShow()) buses[0].bus->Show();
  }
  else if(buses[0].busW) {
      if(buses[0].busW->CanShow()) buses[0].busW->Show();
  }
}

uint8_t interCounter;
void interCallback() {
  if(interCounter > 1) { // XXX figure out on its own whether will overflow, so can just push towards max always
    timer_inter.once_ms((1000/(interFrames+1)) / cfg->dmxHz.get(), interCallback);
  }
  renderInterFrame(); // should def try proper temporal dithering here...
  interCounter--;
}

// unsigned long renderMicros; // seems just over 10us per pixel, since 125 gave around 1370... much faster than WS??

// seems just over 10us per pixel, since 125 gave around 1370... much faster than WS??  or something off with measurement
//keyframe as TARGET: can be overshot; keep it moving
//calculating diff for(color1-color2)/interFrames and simply adding this static amount each inter
//faster + solves issue of uneven interpolation, and never quite arriving at incoming frame
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
	if(universe < start_uni || universe >= start_uni + universes) return; // lib takes care of this no?
  // if(modeNode.) // is set to dmx control. tho just kill then instead prob and never reach here

  static int first = millis();
  static long dmxFrameCounter = 0;
  static bool dmxLed = false;
  dmxFrameCounter++;

  if(!(dmxFrameCounter % 10)) {
    statusBrightness += (dmxLed? 10: -10);
    if(statusBrightness <= 10 || statusBrightness > 245) dmxLed = !dmxLed;
    statusRing.SetBrightness(statusBrightness);
    statusRing.Show();
    // LN.logf("DMX", LoggerNode::DEBUG, "statusBrightness: %d", statusBrightness);

    // modeNode.setProperty("brightness").send(String(statusBrightness));
    // brightnessOverride = statusBrightness;
  }
  if(dmxFrameCounter >= cfg->dmxHz.get() * 10) {
    int totalTime = 0;
    totalTime = millis() - first;
      if(length > 12) {
        LN.logf("DMX", LoggerNode::INFO, "avg %dhz, Uni%d, fns %d, %d %d, %d/%d, %d %d, %d/%d, %d/%d", dmxFrameCounter / (totalTime / 1000),
            universe, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]);
      }
      dmxFrameCounter = 0;
      first = millis();
      LN.logf("brightness", LoggerNode::DEBUG, "b var: %d, override: %d, out: %d",
          brightness, brightnessOverride, outBrightness);
  }
	if(log_artnet >= 2) Homie.getLogger() << universe;

	uint8_t* functions = &data[-1];  // take care of DMX ch offset...

  // all this should see is data is handed to one or more outputs
  // Strip, SerialDMX (via max485), non-IP wireless, a fadecandy?/any hw/software endpoint with
  // no fn ch capability so pixtol runs as basically an effect box?  or sniffer/tester
  // so might send as artnet cause actually came in by XLR, or fwd wifi by CAT or versa? neither end shall give any damns
  //
  // these should call strip/output directly
    updatePixels(data); //XXX gotta send kength along too really no?
    updateFunctions(functions, true);

    // this should be a scheduler calling outputter to actually render/write
    // frames can and do stutter, interpolation and minor buffering can help that
    // plus remember animation retiming off music tempo etc, that's for like a pi tho
    // + like "if safe mode: check each pixel brightness, calculate approx total mA, dim all if needed (running off usb, straight offa ESP etc)"

    if(buses[0].bus) {
      if(buses[0].bus->CanShow()) buses[0].bus->Show();
    }
    else if(buses[0].busW) {
      if(buses[0].busW->CanShow()) {
          buses[0].busW->Show();
      }
    }

    // above entity should keep track of all incoming data sources and merge appropriately
    // depending on prio, LTP/HTP/mymuchbetterideaofweightedaverages
    // and schedule interpolation
    memcpy(last_data, data, sizeof(uint8_t) * 512);
    interCounter = interFrames;
    // 1000/interFrames / hz is wrong no?  if 1 inter 40 hz then should be at 12.5 ms, not 25, so add 1? 
    if(interCounter) timer_inter.once_ms((1000/(interFrames+1)) / cfg->dmxHz.get(), interCallback);
  }

void loopArtNet() {
  doNodeReport();
  artRDM.handler();
  
  yield();

  switch(artnet.read()) { // check opcode for logging purposes, actual logic in callback function
      LN.logf("loopArtNet()", LoggerNode::INFO, "whatever");
    case OpDmx:
      if(log_artnet >= 4) Homie.getLogger() << "DMX ";
      break;
    case OpPoll:
      if(log_artnet >= 2) Homie.getLogger() << "Art Poll Packet";
      LN.logf("loopArtNet()", LoggerNode::INFO, "Art Poll Packet");
      break;
  }
}

  // int countMicros = 0;
// int iterations = 0;

void loop() {
  // static int countMillis = 0;
  // static int iterations = 0;

  // static int last = 0;
  // last = micros();
  loopArtNet(); // if this doesn't result in anything, or mode aint dmx, do a timer/callback based loop

  // XXX Idea: every 10s grab a frame. save last minute or so. mqtt/dmx ctrl both to manually save a frame
  // and to grab frame afterthefact from that buffer.
  // then do the animation fades between saved frames

  // XXX write bluetooth receiver for raspberry, can also send commands that way (fwded to esps by wifi)
	Homie.loop();
  if(Homie.isConnected()) { // kill any existing go-into-wifi-finder timer, etc
    ArduinoOTA.handle(); // soon entirely redundant due to homie
  } else {
    // stays stuck in this state for a while on boot
    // LN.logf("loop()", LoggerNode::INFO, "OMG NO CONNECT");
    // set a timer if not already exists, yada yada blink statusled for sure...
  }

//   iterations++;
//   countMicros += micros() - last;
//   if (iterations >= 1000000) {
//     LN.logf("loop()", LoggerNode::INFO, "1000000 loop avg: %d micros", countMicros / 1000000);
//     countMicros = 0;
//     iterations = 0;
//  }
  // make something proper so can estimate interpolation etc, and never crashing from getting overwhelmed...
}
