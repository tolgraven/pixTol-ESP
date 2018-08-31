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
uint8_t interFrames; // ~40 us/led gives 5 ms for 1 universe of 125 leds. mirrored 144 rgb a 30 is 9 so 1-2 inters  should be alright.


int ctrl[25] = {-1}; // 1-12 ctrl, 13-24 blend mqtt vs dmx on that ch, 25 tot blend?

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

bool settingsHandler(const HomieRange& range, const String& value) {
  modeNode.setProperty("settings").send(value);
  return true; // was thinking interframes, strobehz, dmxhz, log, flipped. but eh so much wooorke
}

void initHomie() {
  Homie_setFirmware(FW_NAME, FW_VERSION); Homie_setBrand(FW_BRAND);
	Homie.setConfigurationApPassword(FW_NAME);

  // Homie.setLedPin(D2, HIGH); Homie.disableLedFeedback();
  Homie.onEvent(onHomieEvent);
  Homie.setGlobalInputHandler(globalInputHandler).setBroadcastHandler(broadcastHandler);
  // Homie.setSetupFunction(homieSetup).setLoopFunction(homieLoop);

	modeNode.advertiseRange("controls", 1, 25).settable(controlsHandler);
	modeNode.advertise("color").settable(colorHandler);
	modeNode.advertise("settings").settable(settingsHandler);

  cfg = new ConfigNode();

  outputNode.advertise("strip").settable();
  inputNode.advertise("artnet").settable();
  statusNode.advertise("freeHeap"); statusNode.advertise("vcc");
  statusNode.advertise("fps"); statusNode.advertise("droppedFrames");

  if(analogRead(0) > 100) battery = new BatteryNode(); // hella unsafe, should be a setting
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
}

void updateFunctions(uint8_t* functions) {
  for(uint8_t i=1; i < f->numChannels; i++) {
    if(ctrl[i] >= 0) {
      // float blend = ctrl[i + f->numChannels] / 255.0f; // second half of array is (inverse) blend fractions
      // functions[i] = (uint8_t)ctrl[i] * blend + functions[i] * (1.0f - blend);
    } //snabb fulhack. Generally these would be set appropriately (or from settings) at boot and used if no ctrl values incoming
  } //but also allow (with prio/mode flag) override etc. and adjust behavior, why have anything at all hardcoded tbh
    //obvs opt to remove dmx ctrl chs as well for 3/4 extra leds wohoo

  f->update(functions, *s);

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
    timer_inter.once_ms((1000/(interFrames+1)) / dmxHz, interCallback);
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
    /* statusNode.setProperty("freeHeap").send(String(ESP.getFreeHeap())); */ //XXX only send if below previous value, or below 20k?
    /* statusNode.setProperty("vcc").send(String(ESP.getVcc())); */ //again send at start, then only if seems fucked up
    statusNode.setProperty("fps").send(String(dmxFrameCounter / (totalTime / 1000))); //possibly restrict to if subst below cfg hz
    statusNode.setProperty("droppedFrames").send(String(s->droppedFrames)); //look at average, how many
    /* statusNode.setProperty("ctrl").setRange(); //and so on, dump that shit out data[1], data[2], data[3], data[4], // data[5], data[6], data[7], data[8], data[9], data[10] */

    LN.logf("brightness", LoggerNode::DEBUG, "var %d, force %d, out %d",
                          f->dimmer.brightness, ctrl[chDimmer], f->outBrightness);

    dmxFrameCounter = 0;
  }
	if(log_artnet >= 2) Homie.getLogger() << universe;
}

// unsigned long renderMicros; // seems just over 10us per pixel, since 125 gave around 1370... much faster than WS??
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {

    updatePixels(data + f->numChannels); //XXX gotta send length along too really no?
    updateFunctions(data);  // take care of DMX ch offset...
    s->show();

    memcpy(targetBuffer, data + f->numChannels, sizeof(uint8_t) * 512);
    interCounter = interFrames;
    if(interCounter) timer_inter.once_ms((1000/(interFrames+1)) / dmxHz, interCallback);

    logDMXStatus(universe, data, length);
  }

void loopArtNet() {
  switch(artnet.read()) { // check opcode for logging purposes, actual logic in callback function
    case OpDmx:
      if(log_artnet >= 4) // Homie.getLogger() << "DMX ";
      break;
    case OpPoll:
      if(log_artnet >= 2) LN.logf("artnet", LoggerNode::DEBUG, "Art Poll Packet");
      break;
  }
}

void loop() {
  static int countMicros = 0;
  // static int iterations = 0;
  static int last = micros();

  loopArtNet(); // skip callback stuff and just use .read once move to class? prob makes more sense

  ArduinoOTA.handle(); // soon entirely redundant due to homie
	Homie.loop();
  if(Homie.isConnected()) { // kill any existing go-into-wifi-finder timer, etc
  } else { // stays stuck in this state for a while on boot
    // LN.logf(__func__, LoggerNode::INFO, "OMG NO CONNECT");
    // set a timer if not already exists, yada yada blink statusled for sure...
  }

//   iterations++;
//   countMicros += micros() - last;
//   if (iterations >= 1000000) {
//     LN.logf(__func__, LoggerNode::INFO, "1000000 loop avg: %d micros", countMicros / 1000000);
//     countMicros = 0;
//     iterations = 0;
//  } // make something proper so can estimate interpolation etc, and never crashing from getting overwhelmed...
}
