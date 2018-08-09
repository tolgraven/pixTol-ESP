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
uint16_t led_count, source_led_count;
bool mirror, folded, flipped;
bool gammaCorrect;
uint8_t log_artnet;
bool debug;
uint8_t hzMin, hzMax;
uint8_t dmxHz;

uint16_t droppedFrames =  0;

NeoGamma<NeoGammaTableMethod> *colorGamma;
Strip* bus;

uint8_t last_data[512] = {0};
uint8_t* last_functions = &last_data[-1];  // take care of DMX ch offset...

Ticker timer_inter;
uint8_t interFrames; // ~40 us/led gives 5 ms for 1 universe of 125 leds. mirrored 144 rgb a 30 is 9 so 1-2 inters
// so around 4-5 frames per frame should be alright.  XXX NOT if mirrored remember! make dynamic!!!
float blendBaseline;

uint8_t brightness;
uint8_t last_brightness = 0;
int brightnessOverride = -1;
uint8_t outBrightness;
float attack, rls, dimmer_attack, dimmer_rls;

BitBangGRB statusRing(D4, 12);
uint8_t statusBrightness = 15;


bool brightnessHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("brightness").send(value);
  brightnessOverride = value.toInt();
  bus->SetBrightness(brightnessOverride);
  bus->Show();
  return true;
}
bool colorHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("color").send(value);
  if(value.equals("blue")) {
    bus->SetColor(blue);
  }
  bus->Show();
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
	modeNode.advertise("brightness").settable(brightnessHandler);
	modeNode.advertise("color").settable(colorHandler);

  battery = new BatteryNode();

	Homie.setup();
}


void setup() {
	Serial.begin(SERIAL_BAUD);
	while(!Serial);
  randomSeed(analogRead(0));

  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  initHomie();

  interFrames = cfg->interFrames.get();
  blendBaseline = 1.00f - 1.00f / (1 + interFrames);
  attack = blendBaseline;
  rls = blendBaseline;
  dimmer_attack = 1.00f - blendBaseline;
  dimmer_rls = 1.00f - blendBaseline;

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

  hzMin = 0.1; //cfg->strobeHzMin.get();
  hzMax = 6.0; //cfg->strobeHzMax.get();  // should these be setable through dmx control ch?                                                      
  dmxHz = cfg->dmxHz.get();
  // start_uni = cfg->startUni.get();
  universes = 1; //cfg->universes.get();

  if(cfg->clearOnStart.get()) {
    DmaGRBW tempbus(288);
    tempbus.Begin();
    tempbus.ClearTo(black);
    tempbus.Show();
    delay(20); // no idea why the .Show() doesn't seem to take without this? somehow doesnt latch or something?
    yield();
    Homie.getLogger() << "Cleared >1 universe of LEDs" << endl;
  }

  bus = new Strip(Strip::PixelBytes(cfg->bytesPerLed.get()), cfg->ledCount.get());

	initArtnet(Homie.getConfiguration().name, universes, cfg->startUni.get(), onDmxFrame);
	setupOTA(led_count);

  if(debug) {
    Homie.getLogger() << "Chance to flash OTA before entering main loop";
    for(int8_t i = 0; i < 5; i++) {
      ArduinoOTA.handle(); // give chance to flash new code in case loop is crashing
      delay(20);
      Homie.getLogger() << ".";
      yield();
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
    if(pixel % 2) { // if uneven
      pixel /= 2;  // since every other pixel is from opposite end
    } else {
      pixel = led_count-1 - pixel/2;
    }
  }
  if(flipped) {
    pixel = cfg->ledCount.get()-1 - pixel;
  }
  return pixel;
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
	iStripDriver* busD = bus->driver;
  float brightnessFraction = 255 / (brightness? brightness: 1); // ahah can't believe divide by zero got me

  for(int t = DMX_FN_CHS; t < data_size; t += bytes_per_pixel) {

    pixelidx = getPixelIndex(pixel);
    uint8_t maxNoise = 48; //baseline, 10 either direction
    if(functions[CH_NOISE]) maxNoise = (1 + functions[CH_NOISE]) / 4 + maxNoise; // CH_NOISE at max gives +-48

    uint8_t subPixel[bytes_per_pixel];
    for(uint8_t i=0; i < bytes_per_pixel; i++) {
      if(iteration == 1) subPixelNoise[pixelidx][i] = random(maxNoise) - maxNoise/2;
      else if(iteration >= interFrames * 10) iteration = 0;
      iteration++;

      subPixel[i] = data[t+i];
      if(subPixel[i] > 16) {
        int16_t tot = subPixel[i] + subPixelNoise[pixelidx][i];
        subPixel[i] = (tot < 0? 0: (tot > 255? 255: tot));
      }
    }
    // wrap colors same way as strip then just create base pointer, instantiate inside
    // if, then can run all CalculateBrightness, SetPixelColor etc generically
    
		if(bytes_per_pixel == 3) { // when this is moved to input->pixelbuffer stage there will be multiple configs: format of input (RGB, RGBW, HSL etc) and output (WS/SK). So all sources can be used with all endpoints.
			RgbColor color = RgbColor(subPixel[0], subPixel[1], subPixel[2]);
      RgbColor lastColor; 
      busD->GetPixelColor(pixelidx, lastColor);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
      color = RgbColor::LinearBlend(color, lastColor, (brighter ? attack : rls));
			if(color.CalculateBrightness() < 8) color.Darken(8); // avoid bitcrunch XXX should rather flag pixel for temporal dithering yeah?

      busD->SetPixelColor(pixelidx, color);
			if(mirror) busD->SetPixelColor(led_count-1 - pixelidx, color);   // XXX offset colors against eachother here! using HSL even, (one more saturated, one lighter).  Should bring a tiny gradient and look nice (compare when folded strip not mirrored and loops back with glorious color combination results)

		} else if(bytes_per_pixel == 4) {
			RgbwColor color = RgbwColor(subPixel[0], subPixel[1], subPixel[2], subPixel[3]);
      RgbwColor lastColor;
      busD->GetPixelColor(pixelidx, lastColor);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
      // XXX need to actually restore lastColor to its original brightness before mixing tho...
      color = RgbwColor::LinearBlend(color, lastColor, (brighter? attack: rls));
			if(color.CalculateBrightness() < 8) color.Darken(8);

      if(gammaCorrect) color = colorGamma->Correct(color); // test. better response but fucks resolution a fair bit. Also wrecks saturation? and general output. Fuck this shit

      busD->SetPixelColor(pixelidx, color);
			if(mirror) busD->SetPixelColor(led_count-1 - pixelidx, color);   // XXX offset colors against eachother here! using HSL even, (one more saturated, one lighter).  Should bring a tiny gradient and look nice (compare when folded strip not mirrored and loops back with glorious color combination results)

		} pixel++;
	}

  if(functions[CH_BLEED]) { // loop through again...
    if(bytes_per_pixel == 3) return; // XXX would crash below when RGB and no busW...

    for(pixel = 0; pixel < led_count; pixel++) {
			RgbwColor color;
      busD->GetPixelColor(getPixelIndex(pixel), color);
			uint8_t thisBrightness = color.CalculateBrightness();
      RgbwColor prevPixelColor = color; // same concept just others rub off on it instead of other way...
      RgbwColor nextPixelColor = color;
			float prevWeight, nextWeight;

      if(pixel-1 >= 0) {
				RgbwColor prevPixelColor;
        busD->GetPixelColor(getPixelIndex(pixel-1), prevPixelColor);
				prevWeight = prevPixelColor.CalculateBrightness() / thisBrightness+1;
				if(prevPixelColor.CalculateBrightness() < thisBrightness * 0.5) // skip if dark, test
					prevPixelColor = color;
			}

      if(pixel+1 < led_count) {
				RgbwColor nextPixelColor;
        busD->GetPixelColor(getPixelIndex(pixel-1), nextPixelColor);
				nextWeight = nextPixelColor.CalculateBrightness() / thisBrightness+1;
				if(nextPixelColor.CalculateBrightness() < thisBrightness * 0.5) // skip if dark, test
					nextPixelColor = color;
					// do some more shit tho. Important thing is mixing colors, so maybe look at saturation as well as brightness dunno
					// since we have X and Y should weight towards more interesting.
			}
      float amount = (float)functions[CH_BLEED]/(256*2);  //this way only ever go half way = before starts decreasing again
      color = RgbwColor::BilinearBlend(color, nextPixelColor, prevPixelColor, color, amount, amount);

      busD->SetPixelColor(getPixelIndex(pixel), color); // XXX handle flip and that

    }
  }
	if(functions[CH_HUE]) { // rotate hue around, simple. global desat as well?

	}
}

void updateFunctions(uint8_t* functions) {

  static uint8_t ch_strobe_last_value = 0;
  static bool shutterOpen = true;
  // XXX strobe existing anim by turn down lightness/using HTP for two sources, fix so can do same straight from afterglow
  if(functions[CH_STROBE]) {
    static uint8_t strobeTickerClosed, strobeTickerOpen;
    static uint16_t onTime, strobePeriod;
    static const float onFraction = 5;

    if(functions[CH_STROBE] != ch_strobe_last_value) { // reset timer for new state

      float hz = (hzMax-hzMin) * (functions[CH_STROBE]-1) / (255-1) + hzMin;
      strobePeriod = 1000 / hz;   // 1 = 1 hz, 255 = 10 hz, to test
      onTime = strobePeriod / onFraction;
      if(onTime > 150) onTime = 120;
      strobeTickerOpen = interFrames * onTime / dmxHz; // take heed of interframes...
      strobeTickerClosed = interFrames * (strobePeriod - onTime) / dmxHz; // since we only decrease one at a time it needs to _add up_ to strobePeriod no?
      shutterOpen = false;
    } else { // handle running strobe: d1cr/reset tickers, adjust shutter

      if(shutterOpen) strobeTickerOpen--;
      else            strobeTickerClosed--;
      if(!strobeTickerClosed) {
        shutterOpen = true;
        strobeTickerClosed = interFrames * (strobePeriod - onTime) / dmxHz;
      } else if(!strobeTickerOpen) {
        shutterOpen = false;
        strobeTickerOpen = interFrames * onTime / dmxHz;
      }
    }
  } else { // 0, clean up
    shutterOpen = true;
  }
  ch_strobe_last_value = functions[CH_STROBE];

  if(functions[CH_ROTATE_FWD]) { // if(functions[CH_ROTATE_FWD] && isKeyframe) {
		bus->driver->RotateRight(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
  // } if(isKeyframe && functions[CH_ROTATE_BACK] && (functions[CH_ROTATE_BACK] != last_functions[CH_ROTATE_BACK])) { // so what happens is since this gets called in interpolation it shifts like five extra times?
  } if(functions[CH_ROTATE_BACK]) { // very cool kaozzzzz strobish when rotating strip folded, 120 long but defed 60 in afterglow. explore!!
		bus->driver->RotateLeft(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
  }

  //WTF: when not enough current and strips yellow, upping atttack makes them less yellow. Also happens in general, but less noticable?
  if(functions[CH_ATTACK] != last_functions[CH_ATTACK]) {
    attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_ATTACK]/295); // dont ever quite arrive like this, two runs at max = 75% not 100%...
    LN.logf("updateFunctions", LoggerNode::DEBUG, "Attack: %d / %f", functions[CH_ATTACK]);
  }
  if(functions[CH_RELEASE] != last_functions[CH_RELEASE]) {
      rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_RELEASE]/265);
  }

  if(functions[CH_DIMMER_ATTACK] != last_functions[CH_DIMMER_ATTACK]) {
      // dimmer_attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_ATTACK]/285); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      dimmer_attack = (1.00f - blendBaseline) - (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_ATTACK]/315); // dont ever quite arrive like this, two runs at max = 75% not 100%...
  }
  if(functions[CH_DIMMER_RELEASE] != last_functions[CH_DIMMER_RELEASE]) {
      dimmer_rls = (1.00f - blendBaseline) - (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_RELEASE]/315);
  }

  if(brightnessOverride <= 0) {
    bool brighter = brightness > last_brightness;
    // LN.logf("brightness", LoggerNode::DEBUG, "%d + (%d - %d) * %f", last_brightness, functions[CH_DIMMER], last_brightness, dimmer_attack);
    float diff = (functions[CH_DIMMER] - last_brightness) * (brighter ? dimmer_attack : dimmer_rls);
    if(diff > 0.1f && diff < 1.0f) diff = 1.0f;
    brightness = last_brightness + diff; // crappy workaround, store dimmer internally as 16bit or float instead.
    if(brightness < 4) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
    else if(brightness > 255) brightness = 255;

  } else if(brightnessOverride <= 255){
    brightness = brightnessOverride;
  }
  last_brightness = brightness;
  outBrightness = (shutterOpen? brightness: 0);

  bus->driver->SetBrightness(outBrightness);
}

void renderInterFrame() {
  updatePixels(last_data);
  updateFunctions(last_functions, false); // XXX fix so interpolates!!
  if(!bus->Show()) droppedFrames++;
}

uint8_t interCounter;
void interCallback() {
  if(interCounter > 1) { // XXX figure out on its own whether will overflow, so can just push towards max always
    timer_inter.once_ms((1000/(interFrames+1)) / cfg->dmxHz.get(), interCallback);
  }
  renderInterFrame(); // should def try proper temporal dithering here...
  interCounter--;
}

void logDMXStatus(uint16_t universe, uint8_t* data, uint16_t length) { // for logging and stuff... makes sense?

  static int first = millis();
  static long dmxFrameCounter = 0;
  dmxFrameCounter++;

  if(!(dmxFrameCounter % 10)) {
  }
  if(!(dmxFrameCounter % (dmxHz*10))) { // every 10s (if input correct and stable)
    uint16_t totalTime = millis() - first;
    first = millis();
    /* LN.logf("DMX", LoggerNode::INFO, "dmxFrameCounter: %d, dmxHz: %d, dmxHz*10: %d, totalTime: %d", */
    /*     dmxFrameCounter, dmxHz, dmxHz * 10, totalTime); */
    LN.logf("DMX", LoggerNode::INFO, "Artnet buffer addr: %d, Strip buffer addr: %d",
        artnet.getDmxFrame(), bus->driver->Pixels());

    if(length > 12) {
      // LN.logf("DMX", LoggerNode::INFO,
      //   // "U-%d %dhz, %d, %d %d, %d/%d, %d %d, %d/%d, %d/%d, drop %d frames, %d free heap, %d vcc",
      //   "U-%d %dhz, %d, %d %d, %d/%d, %d %d, %d/%d, %d/%d, drop %d frames",
      //   universe, dmxFrameCounter / (totalTime / 1000), data[0], data[1], data[2], data[3], data[4],
      //   // data[5], data[6], data[7], data[8], data[9], data[10], droppedFrames, ESP.getFreeHeap(), ESP.getVcc());
      //   data[5], data[6], data[7], data[8], data[9], data[10], droppedFrames);
      LN.logf("brightness", LoggerNode::DEBUG, "var %d, force %d, out %d",
            brightness, brightnessOverride, outBrightness);
    } else {
      LN.logf("DMX", LoggerNode::ERROR, "Frame length short, %d", length);
    }

    dmxFrameCounter = 0;
  }
	if(log_artnet >= 2) Homie.getLogger() << universe;
}

	uint8_t* functions = &data[-1];  // take care of DMX ch offset...

  // all this should see is data is handed to one or more outputs
  // Strip, SerialDMX (via max485), non-IP wireless, a fadecandy?/any hw/software endpoint with
  // no fn ch capability so pixtol runs as basically an effect box?  or sniffer/tester
  // so might send as artnet cause actually came in by XLR, or fwd wifi by CAT or versa? neither end shall give any damns

  // ^^ WRONG. First pass to intermediate buffer, eg got multiple unis = dont flush til got all
  // also easier to manipulate rotation etc BEFORE mapping onto strip, so dont have to convert back and forth

    updatePixels(data); //XXX gotta send length along too really no?
    updateFunctions(functions, true);

    // this should be a scheduler calling outputter to actually render/write
    // frames can and do stutter, interpolation and minor buffering can help that
    // plus remember animation retiming off music tempo etc, that's for like a pi tho
    // + like "if safe mode: check each pixel brightness, calculate approx total mA, dim all if needed (running off usb, straight offa ESP etc)"

    if(!bus->Show()) droppedFrames++;

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
    case OpDmx:
      if(log_artnet >= 4) Homie.getLogger() << "DMX ";
      break;
    case OpPoll:
      if(log_artnet >= 2) Homie.getLogger() << "Art Poll Packet";
      // LN.logf("artnet", LoggerNode::DEBUG, "Art Poll Packet");
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
