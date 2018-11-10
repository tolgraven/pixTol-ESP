#include "state.h"

ConfigNode* cfg;
BatteryNode* battery;
Strip* s;
Blinky* b;
Functions* f;

uint16_t start;
uint32_t keyFrameInterval, gotFrameAt; // target interval between keyframes, abs time (micros) last frame read

Buffer* targetFunctions;
PixelBuffer* targetBuffer;
PixelBuffer* wasBuffer;
// uint8_t buffers[3][512] = {0}; // prev, target, discard buffers
// we'd have, prev, target, output?

HomieEvent* lastEvent = nullptr;
uint8_t disconnections = 0;

ArtnetnodeWifi* artnet;
int lastArtnetOpCode;

void initArtnet(const String& name, uint8_t numPorts, uint8_t startingUniverse, void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*) ) {
  artnet = new ArtnetnodeWifi();

  artnet->setName(name.c_str());
  artnet->setNumPorts(numPorts);
	artnet->setStartingUniverse(startingUniverse);
	for(int i = 0; i < numPorts; i++) artnet->enableDMXOutput(i);
	artnet->begin();
	artnet->setArtDmxCallback(callback);
}

void initDevice() { // init Serial, random seed, some first boot animation? anything else pre-homie/state?
	Serial.begin(SERIAL_BAUD);
  randomSeed(analogRead(0));
}

void initScheduler() {
  keyFrameInterval = MILLION / cfg->dmxHz.get();
  start = micros();
}

void initState() { //retain something like this turning settings and defaults into actual stuff... prob part of "state" class, getting data from all places keeping track of that it means, and eg writing state from mqtt into json settings...
  s = new Strip("Bad aZZ", LEDS(cfg->stripBytesPerPixel.get()), cfg->stripLedCount.get());
  s->mirror(cfg->setMirrored.get()).fold(cfg->setFolded.get()); // s->flip(false); //no need for now as no setting for it //cfg->setFlipped.get();
  b = new Blinky(s);
  LN.logf(__func__, LoggerNode::DEBUG, "Done strip");

  f = new Functions(MILLION / cfg->dmxHz.get(),
                    BlendEnvelope("pixelEnvelope", (float[]){1.2, 1.0}, true),
                    BlendEnvelope("dimmerEnvelope", (float[]){1.2, 1.2}),
                    s);
  LN.logf(__func__, LoggerNode::DEBUG, "Done functions");

  targetBuffer = new PixelBuffer(cfg->stripBytesPerPixel.get(), cfg->stripLedCount.get());
  targetFunctions = new Buffer("Strip fn target", 1, f->numChannels);
  LN.logf(__func__, LoggerNode::DEBUG, "Done buffer");

  initArduinoOTA(b, s); //initOTA();

  uint8_t universes = 1; //for now anyways
	initArtnet(Homie.getConfiguration().name, universes, cfg->startUni.get(), onDmxFrame);
}
