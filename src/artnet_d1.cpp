#include "artnet_d1.h"

ConfigNode* cfg;
BatteryNode* battery;
Strip* s;
Functions* f;

uint8_t universes;
uint8_t log_artnet;
uint8_t dmxHz;

uint8_t targetBuffer[512] = {0};

Ticker timer_inter;
uint8_t interFrames; // ~40 us/led gives 5 ms for 1 universe of 125 leds. mirrored 144 rgb a 30 is 9 so 1-2 inters
// so around 4-5 frames per frame should be alright.  XXX NOT if mirrored remember! make dynamic!!!
float blendBaseline;


Functions* f;

int ctrl[12] = {-1};

bool controlsHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("controls").setRange(range).send(value);
  ctrl[range.index] = value.toInt();
  return true;
}

bool colorHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("color").send(value);
  if(value == "blue")       s->setColor(colors.blue);
  else if(value == "green") s->setColor(colors.green);
  else if(value == "red")   s->setColor(colors.red);

  return true;
}


void initHomie() {
  Homie_setFirmware(FW_NAME, FW_VERSION); Homie_setBrand(FW_BRAND);
	Homie.setConfigurationApPassword(FW_NAME);

  // Homie.setLedPin(D2, HIGH); Homie.disableLedFeedback();
  Homie.onEvent(onHomieEvent);
  Homie.setGlobalInputHandler(globalInputHandler);
  Homie.setBroadcastHandler(broadcastHandler);
  // Homie.setSetupFunction(homieSetup).setLoopFunction(homieLoop);
	// Homie.setLoggingPrinter(); //only takes Serial objects. Mod for mqtt?

	modeNode.advertiseRange("controls", 1, 12).settable(controlsHandler);
	modeNode.advertise("color").settable(colorHandler);

  cfg = new ConfigNode();

  outputNode.advertise("strip").settable();
  inputNode.advertise("artnet").settable();
  statusNode.advertise("freeHeap"); statusNode.advertise("vcc");
  statusNode.advertise("fps"); statusNode.advertise("droppedFrames");
  statusNode.advertiseRange("ctrl", 1, 12);

  if(analogRead(0) > 100) { // hella unsafe, should be a setting
    battery = new BatteryNode();
  }
  // if(!cfg_debug.get()) Homie.disableLogging();

	Homie.setup();
}

void setupGlobals() { //retain something like this turning settings and defaults into actual stuff... prob part of "state" class, getting data from all places keeping track of that it means, and eg writing state from mqtt into json settings...

  log_artnet = cfg->logArtnet.get();

  dmxHz = cfg->dmxHz.get();
  interFrames = cfg->interFrames.get();
  universes = 1; //cfg->universes.get();
}


void setup() {
  int bootMillis = millis();
  int bootHeap = ESP.getFreeHeap();
	Serial.begin(SERIAL_BAUD);
  randomSeed(analogRead(0));

  initHomie();
  int postHomieHeap = ESP.getFreeHeap();
  LN.logf("Reset info", LoggerNode::DEBUG, "%s", ESP.getResetInfo().c_str());

  testStrip(cfg->bytesPerLed.get(), cfg->ledCount.get());
  setupGlobals();

  float baseline = 1.0 / (1 + interFrames); // basically weight of current state vs incoming when interpolating
  f = new Functions(dmxHz * (interFrames + 1),
      BlendEnvelope(baseline, 60, 20, true), BlendEnvelope(baseline, 50, 50));
  for(uint8_t i=0; i < f->numChannels; i++) ctrl[i] = -1; //uh why do i need this
  for(uint8_t i=f->numChannels; i < f->numChannels*2; i++) ctrl[i] = 127; //half-n-half by default

  if(cfg->clearOnStart.get()) {
    blinkStrip(288, colors.black, 1);
    LN.logf("clearOnStart", LoggerNode::DEBUG, "Cleared >2 universes of LEDs");
  }

  s = new Strip("Bad aZZ", LEDS(cfg->bytesPerLed.get()), cfg->ledCount.get());
  s->setMirrored(cfg->setMirrored.get());
  s->setFolded(cfg->setFolded.get());
  // s->beFlipped = false; //no need for now as no setting for it //cfg->setFlipped.get();

	initArtnet(Homie.getConfiguration().name, universes, cfg->startUni.get(), onDmxFrame);
	setupOTA(s->ledsInStrip);

  if(true) { // debug
    Homie.getLogger() << "Chance to flash OTA before entering main loop";
    for(int8_t i = 0; i < 5; i++) {
      ArduinoOTA.handle(); // give chance to flash new code in case loop is crashing
      delay(20);
      Homie.getLogger() << ".";
      yield();
    }
    Homie.getLogger() << endl;
  }
  LN.logf(__func__, LoggerNode::DEBUG, "Free heap post boot/homie/rest: %d / %d / %d, took %d ms",
      bootHeap, postHomieHeap, ESP.getFreeHeap(), millis() - bootMillis);
}


void updatePixels(uint8_t* data) { // XXX also pass fraction in case interpolating >2 frames
  static uint8_t iteration = 0;
  static int8_t subPixelNoise[125][4] = {0};

	int pixel = 0; //512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
  float brightnessFraction = 255 / (f->dimmer.brightness? f->dimmer.brightness: 1); // ahah can't believe divide by zero got me

  for(int t = 0; t < s->dataLength; t += s->fieldSize) {

    uint8_t maxNoise = 64; //baseline, 10 either direction
    if(f->ch[chNoise]) maxNoise = (1 + f->ch[chNoise]) / 4 + maxNoise; // CH_NOISE at max gives +-48

    uint8_t subPixel[s->fieldSize];
    for(uint8_t i=0; i < s->fieldSize; i++) {
      // if(iteration == 1) subPixelNoise[pixel][i] = random(maxNoise) - maxNoise/2;
      // else if(iteration >= interFrames * 20) iteration = 0;
      // iteration++;

      subPixel[i] = data[t+i];
      if(subPixel[i] > 16) {
        int16_t tot = subPixel[i] + subPixelNoise[pixel][i];
        subPixel[i] = tot < 0? 0: (tot > 255? 255: tot);
      }
    }
    // iColor* color; // wrap colors same as strip then just base pointer, instantiate in if, then run all CalculateBrightness, SetPixelColor etc generically

		if(s->fieldSize == 3) { // when this is moved to input->pixelbuffer stage there will be multiple configs: format of input (RGB, RGBW, HSL etc) and output (WS/SK). So all sources can be used with all endpoints.
			RgbColor color = RgbColor(subPixel[0], subPixel[1], subPixel[2]);
      // color = new ColorRGB(subPixel[0], subPixel[1], subPixel[2]);
      RgbColor lastColor;
      s->getPixelColor(pixel, lastColor);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
      color = RgbColor::LinearBlend(color, lastColor, (brighter ? f->e.attack : f->e.release));

      s->setPixelColor(pixel, color);

		} else if(s->fieldSize == 4) {
			RgbwColor color = RgbwColor(subPixel[0], subPixel[1], subPixel[2], subPixel[3]);
      RgbwColor lastColor;
      s->getPixelColor(pixel, lastColor);  // get from pixelbus since we can resolve dimmer-related changes
      bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
      // XXX would need to actually restore lastColor to its original brightness before mixing tho... lets just double buffer
      color = RgbwColor::LinearBlend(color, lastColor, (brighter? f->e.attack: f->e.release));

      s->setPixelColor(pixel, color);
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
	if(f->ch[chHue]) { // rotate hue around, simple. global desat as well?
    // should be applying value _while looping around pixels and already holding ColorObjects.
    // convert Rgbw>Hsl? can do Hsb to Rgbw at least...
	}
}

void updateFunctions(uint8_t* functions) {
  for(uint8_t i=1; i < DMX_FN_CHS; i++) {
    if(ctrl[i] >= 0) functions[i] = (uint8_t)ctrl[i];
  } //snabb fulhack. Generally these would be set appropriately (or from settings) at boot and used if no ctrl values incoming
    //but also allow (with prio/mode flag) override etc. and adjust behavior, why have anything at all hardcoded tbh
    //obvs opt to remove dmx ctrl chs as well for 3/4 extra leds wohoo

  f->update(functions);

  if(f->ch[chRotateFwd])
    s->rotateFwd(f->val[chRotateFwd]); // these would def benefit from anti-aliasing = decoupling from leds as steps
  if(f->ch[chRotateBack])
    s->rotateBack(f->val[chRotateBack]); // very cool kaozzzzz strobish when rotating strip folded, 120 long but defed 60 in afterglow. explore!!

  s->setBrightness(f->outBrightness);
}

void renderInterFrame() {
  updatePixels(targetBuffer);
  updateFunctions(f->ch); //remember this silly way makes updateFunctions directly fuck w values atm
  s->show();
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

  if(!(dmxFrameCounter % (dmxHz*10))) { // every 10s (if input correct and stable)
    uint16_t totalTime = millis() - first;
    first = millis();
    statusNode.setProperty("freeHeap").send(String(ESP.getFreeHeap()));
    /* statusNode.setProperty("vcc").send(String(ESP.getVcc())); */
    statusNode.setProperty("fps").send(String(dmxFrameCounter / (totalTime / 1000)));
    statusNode.setProperty("droppedFrames").send(String(bus->droppedFrames));
    /* statusNode.setProperty("ctrl").setRange(); //and so on, dump that shit out data[1], data[2], data[3], data[4], // data[5], data[6], data[7], data[8], data[9], data[10] */
    /*     artnet.getDmxFrame(), bus->driver->Pixels()); */

    LN.logf("brightness", LoggerNode::DEBUG, "var %d, force %d, out %d",
                          f->dimmer.brightness, ctrl[chDimmer], f->outBrightness);

    dmxFrameCounter = 0;
  }
	if(log_artnet >= 2) Homie.getLogger() << universe;
}

	uint8_t* functions = &data[-1];  // take care of DMX ch offset...

  // all this should see is data is handed to one or more outputs
  // Strip, SerialDMX (via max485), non-IP wireless, a fadecandy?/any hw/software endpoint with
  // no fn ch capability so pixtol runs as basically an effect box?  or sniffer/tester
  // so might send as artnet cause actually came in by XLR, or fwd wifi by CAT or versa? neither end shall give any damns

    updatePixels(data + f->numChannels); //XXX gotta send length along too really no?
    updateFunctions(data);  // take care of DMX ch offset...
    s->show();

    memcpy(targetBuffer, data + f->numChannels, sizeof(uint8_t) * 512);
    interCounter = interFrames;
    if(interCounter) timer_inter.once_ms((1000/(interFrames+1)) / dmxHz, interCallback);

    logDMXStatus(universe, data, length);
  }

void loopArtNet() {
  doNodeReport();
  artRDM.handler();
  
  yield();

  switch(artnet.read()) { // check opcode for logging purposes, actual logic in callback function
    case OpDmx:
      if(log_artnet >= 4) // Homie.getLogger() << "DMX ";
      break;
    case OpPoll:
      if(log_artnet >= 2) // LN.logf("artnet", LoggerNode::DEBUG, "Art Poll Packet");
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
