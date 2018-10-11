#include "artnet_d1.h"

void setup() {
  int bootMillis = millis();
  int bootHeap = ESP.getFreeHeap();
	Serial.begin(SERIAL_BAUD);
  randomSeed(analogRead(0));

  initHomie();
  int postHomieHeap = ESP.getFreeHeap();
  LN.logf("Reset info", LoggerNode::DEBUG, "%s", ESP.getResetInfo().c_str());

  blinkStrip(288, colors.black, 1);
  testStrip(cfg->stripBytesPerPixel.get(), cfg->stripLedCount.get());

  initState();

	initArtnet(Homie.getConfiguration().name, universes, cfg->startUni.get(), onDmxFrame);
	setupOTA(s->ledsInStrip); //should prob contain below code as well...?

  LN.logf(__func__, LoggerNode::DEBUG, "Free heap post boot/homie/rest: %d / %d / %d, took %d ms",
      bootHeap, postHomieHeap, ESP.getFreeHeap(), millis() - bootMillis);
}


void updatePixels(uint8_t* data) { // XXX also pass fraction in case interpolating >2 frames

	int pixel = 0; //512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
  float brightnessFraction = 255 / (f->dimmer.brightness? f->dimmer.brightness: 1); // ahah can't believe divide by zero got me

  for(int t = 0; t < s->dataLength; t += s->fieldSize) {
    uint8_t subPixel[s->fieldSize];
    for(uint8_t i=0; i < s->fieldSize; i++) {
      subPixel[i] = data[t+i];
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
      float blend = ctrl[i + f->numChannels] / 255.0f; // second half of array is (inverse) blend fractions
      functions[i] = (uint8_t)ctrl[i] * blend + functions[i] * (1.0f - blend);
    } // Generally these would be set appropriately (or from settings) at boot and used if no ctrl values incoming
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
    interTimer->once_ms((1000/(interFrames+1)) / dmxHz, interCallback);
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
  switch(lastArtnetCode) { // check opcode for logging purposes, actual logic in callback function
    case OpDmx:
      if(log_artnet >= 4) Homie.getLogger() << "DMX ";
      break;
    case OpPoll:
      if(log_artnet >= 2) LN.logf("artnet", LoggerNode::DEBUG, "Art Poll Packet");
      break;
  }
	if(log_artnet >= 2) Homie.getLogger() << universe;
}

// unsigned long renderMicros; // seems just over 10us per pixel, since 125 gave around 1370... much faster than WS??
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {

    updatePixels(data + f->numChannels); //XXX gotta send length along too really no?
    updateFunctions(data);  // take care of DMX ch offset...
    s->show();

    // memcpy(targetBuffer, data + f->numChannels, sizeof(uint8_t) * f->numChannels);
    memcpy(targetBuffer, data + f->numChannels, sizeof(uint8_t) * (length - f->numChannels));
    interCounter = interFrames;
    if(interCounter) interTimer->once_ms((1000/(interFrames+1)) / dmxHz, interCallback);

    logDMXStatus(universe, data, length);
  }

void loop() {
  // static int iterations = 0;
  static int countMicros = 0;
  static int last = micros();

  lastArtnetCode = artnet.read(); // skip callback stuff and just use .read once move to class? prob makes more sense

  ArduinoOTA.handle(); // redundant once homie's ota stops being a buggy pos...
	Homie.loop();
  if(Homie.isConnected()) { // kill any existing go-into-wifi-finder timer, etc
  } else { // stays stuck in this state for a while on boot
    // set a timer if not already exists, yada yada blink statusled for sure...
  }
  // run renderer update fn, interpolating as far as needed based on time between loop() invocations

//   iterations++;
//   countMicros += micros() - last;
//   if (iterations >= 1000000) {
//     LN.logf(__func__, LoggerNode::INFO, "1000000 loop avg: %d micros", countMicros / 1000000);
//     countMicros = 0;
//     iterations = 0;
//  } // make something proper so can estimate interpolation etc, and never crashing from getting overwhelmed...
}
