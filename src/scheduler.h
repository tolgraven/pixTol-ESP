#pragma once

#include <vector>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>


// freeze buffer / background thing, run constant at lowest prio whether static or
// last frame from w/e source, could also interpolate between a few frames or have basic
// wave animation etc
// as higher layers of input can be nuking stuff from this (overwriting, or non-HTP taking away)
// something blabla whateveS

class Scheduler {
  std::vector<Inputter*> input;
  std::vector<Outputter*> output;
  Renderer* renderer;

  std::vector<NeoPixelAnimator*> animations;
  // or rather
  // std::vector<Task*> tasks;

  public:
  Scheduler() {
    input.push_back(new sAcnInput("pixTol-sACN", iot->cfg->startUni.get(), 1));
    input.push_back(new ArtnetInput("pixTol-ArtNet", iot->cfg->startUni.get(), 1));

    output.push_back(new Strip("Bad aZZ", iot->cfg->stripBytesPerPixel.get(), iot->cfg->stripLedCount.get()));
    output[0]->mirror(iot->cfg->mirror.get()).fold(iot->cfg->fold.get()); // s->flip(false); //no need for now as no setting for it //iot->cfg->setFlipped.get();

    // inputter.push_back(new OPC(s->fieldCount()));
    keyFrameInterval = MILLION / iot->cfg->dmxHz.get();
    start = micros();
  }

  uint16_t start;
  uint32_t keyFrameInterval, gotFrameAt; // target interval between keyframes, abs time (micros) last frame read

  Scheduler& addInput(Inputter* in) {
    input.push_back(in);
    return *this;
  }
  Scheduler& addOutput(Outputter* out) {
    output.push_back(out);
    return *this;
  }
// unsigned long renderMicros; // seems just over 10us per pixel, since 125 gave around 1370... much faster than WS??
//
  // static int iterations = 0, countMicros = 0, last = micros();
//   iterations++;
//   countMicros += micros() - last;
//   if (iterations >= MILLION) {
//     lg.logf(__func__, _INFO, "1000000 loop avg: %d micros", countMicros / 1000000);
//     countMicros = 0;
//     iterations = 0;
//  } // make something proper so can estimate interpolation etc, and never crashing from getting overwhelmed...
//
  // static uint32_t updates = 0, priorSeconds = 0;
  // uint32_t seconds;
  // if((seconds = (t / MILLION)) != priorSeconds) { // 1 sec elapsed?
  //   lg.logf(__func__, Log::DEBUG, "%d updates/sec", updates);
  //   priorSeconds = seconds;
  //   updates = 0; // Reset counter
  // }
}
