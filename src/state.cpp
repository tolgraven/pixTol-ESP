#include "state.h"

IOT* iot;
Strip* s;
Blinky* b;
Functions* f;

Updater* ota;
Updater* homieUpdater;

uint16_t start;
uint32_t keyFrameInterval, gotFrameAt; // target interval between keyframes, abs time (micros) last frame read

Buffer* targetFunctions;
PixelBuffer* targetBuffer;
PixelBuffer* wasBuffer;
// uint8_t buffers[3][512] = {0}; // prev, target, discard buffers
// we'd have, prev, target, output?


void initDevice() { // init Serial, random seed, some first boot animation? anything else pre-homie/state?
	Serial.begin(SERIAL_BAUD);
  randomSeed(analogRead(0));
}

void initIOT() {
  iot = new IOT("pixTol");
}

void initScheduler() {
  keyFrameInterval = MILLION / iot->cfg->dmxHz.get();
  start = micros();
}

void initState() { //retain something like this turning settings and defaults into actual stuff... prob part of "state" class, getting data from all places keeping track of that it means, and eg writing state from mqtt into json settings...
  s = new Strip("Bad aZZ", iot->cfg->stripBytesPerPixel.get(), iot->cfg->stripLedCount.get());
  s->mirror(iot->cfg->mirror.get()).fold(iot->cfg->fold.get()); // s->flip(false); //no need for now as no setting for it //iot->cfg->setFlipped.get();
  b = new Blinky(s);
  LN.logf(__func__, LoggerNode::DEBUG, "Done strip");

  f = new Functions(MILLION / iot->cfg->dmxHz.get(), s);
  LN.logf(__func__, LoggerNode::DEBUG, "Done functions");

  iot->finishSetup(s, b, f);

  // initArduinoOTA(b, s); //initOTA();

  inputter.push_back(new sAcnInput("pixTol-sACN", iot->cfg->startUni.get(), 1));
  inputter.push_back(new ArtnetInput("pixTol-ArtNet", iot->cfg->startUni.get(), 1));
  // inputter.push_back(new OPC(s->fieldCount()));
  LN.logf(__func__, LoggerNode::DEBUG, "Done inputters");
}
