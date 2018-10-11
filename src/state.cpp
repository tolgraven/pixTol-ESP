#include "state.h"

ConfigNode* cfg;
BatteryNode* battery;
Strip* s;
Functions* f;

Ticker* interTimer;
uint8_t interFrames; // ~40 us/led gives 5 ms for 1 universe of 125 leds. mirrored 144 rgb a 30 is 9 so 1-2 inters  should be alright.

int ctrl[25] = {-1}; // 1-12 ctrl, 13-24 blend mqtt vs dmx on that ch, 25 tot blend?

uint8_t targetBuffer[512] = {0};
uint8_t buffers[3][512] = {0}; // prev, target, discard buffers

uint8_t universes;
uint8_t log_artnet;
int lastArtnetCode;
uint8_t dmxHz;

void initState() { //retain something like this turning settings and defaults into actual stuff... prob part of "state" class, getting data from all places keeping track of that it means, and eg writing state from mqtt into json settings...
  log_artnet = cfg->logArtnet.get();

  dmxHz = cfg->dmxHz.get();
  interFrames = 4; //cfg->interFrames.get(); //4 is aight. wont be needed soon so nuke.
  universes = 1; //cfg->universes.get();

  interTimer = new Ticker();
  float baseline = 1.0 / (1 + interFrames); // basically weight of current state vs incoming when interpolating

  f = new Functions(dmxHz * (interFrames + 1),
      BlendEnvelope(baseline, 60, 20, true), BlendEnvelope(baseline, 50, 50));
  for(uint8_t i=0; i < f->numChannels * 2; i++) {
      ctrl[i] = i < f->numChannels? -1: 127; // first half mirrors channel (-1 = no override), second is blend (how much ev f value from source, how much override - 127 = 0.5)
  }
  LN.logf(__func__, LoggerNode::DEBUG, "Done functions");

  s = new Strip("Bad aZZ", LEDS(cfg->stripBytesPerPixel.get()), cfg->stripLedCount.get());
  s->setMirrored(cfg->setMirrored.get()).setFolded(cfg->setFolded.get());
  // s->setFlipped(false); //no need for now as no setting for it //cfg->setFlipped.get();
  LN.logf(__func__, LoggerNode::DEBUG, "Done strip");
}
