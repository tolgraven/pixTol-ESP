#pragma once

#include <vector>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>

#include "renderer.h"
#include "buffer.h"
#include "io/strip.h"
#include "io/sacn.h"
#include "io/artnet.h"
#include "io/opc.h"
#include "functions.h"
#include "iot.h"
#include "device.h"
#include "watchdog.h"

// freeze buffer / background thing, run constant at lowest prio whether static or
// last frame from w/e source, could also interpolate between a few frames or have basic wave animation etc
// as higher layers of input can be nuking stuff from this (overwriting, or non-HTP taking away)
class Scheduler {
  String _id;
  std::vector<Inputter*> _input;
  std::vector<PixelBuffer*> latest;
  std::vector<Outputter*> _output;
  Renderer* _renderer;
  // std::map<String, SourceData*> sourceData;
  // PixelBuffer* sourceMix;
  PixelBuffer* pb;
  bool newInput = false;
  Field zero; //to zero buffers right?
  int ctrlChannels = 12;
  IOT* _iot = nullptr;

  std::vector<NeoPixelAnimator*> animations; // or rather std::vector<Task*> tasks;

  public:
  Scheduler(const String& id, IOT& iot): _id(id), _iot(&iot) {
    auto& cfg = *_iot->cfg;
    addOutput(new Strip("Strip 1", cfg.stripBytesPerPixel.get(), cfg.stripLedCount.get()));
    static_cast<Strip&>(output(0)).mirror(cfg.mirror.get()).fold(cfg.fold.get()); // s->flip(false); //no need for now as no setting for it //cfg.setFlipped.get();
    lg.dbg("Done add outputs");

    addInput(new sAcnInput(id + " sACN", cfg.startUni.get(), 1));
    addInput(new ArtnetInput(id + " ArtNet", cfg.startUni.get(), 1));
    // addInput(new OPC(s->fieldCount()));
    // addInput(new MqttHomie());
    lg.dbg("Done add inputs");
    // sourceData["ArtNet"] = new SourceData("ArtNet");
    // sourceData["sACN"] = new SourceData("sACN");
    // sourceMix = new PixelBuffer(output(0).fieldSize(), output(0).fieldCount());
    pb = new PixelBuffer(output(0).fieldSize(), output(0).fieldCount());

    keyFrameInterval = MILLION / cfg.dmxHz.get(); //should prob be adaptive in some sense tho... if actual continously different
    start = micros();
    _renderer = new Renderer(keyFrameInterval, output(0));
    ctrlChannels = renderer().f->numChannels;
    lg.dbg("Done create renderer");
  }

  Renderer& renderer() { return *_renderer; }

  Inputter& input(int index) { return *_input[index]; } //XXX bounds
  Inputter& input(const String& type) { for(auto in: _input) if(type == in->type()) return *in; } //watch for if got multiple obj of same type tho... but only likely on outputter end? or physical ports maybe hmm...
  Outputter& output(uint8_t index) { return *_output[index]; }
  Outputter& output(const String& id) { for(auto out: _output) if(id == out->id()) return *out; }
  Scheduler& addInput(Inputter* in) {
    _input.push_back(in);
    // latest.push_back(new PixelBuffer(output(0).fieldSize(), output(0).fieldCount()));
    // ^ XXX temp get specs from cfg or whatever...
    return *this;
  }
  Scheduler& addOutput(Outputter* out) { _output.push_back(out); return *this; }

  uint32_t start, keyFrameInterval, lastKeyframe = 0; // target interval between keyframes, abs time (micros) last frame read

  void handleDMX() {
    lwd.stamp(PIXTOL_SCHEDULER_INPUT_DMX);

    for(auto&& source: _input) { // XXX could something somehow be funky with below after change? so empty source gets used, other not but then how? makes no sense
      // if(!source->run() || !source->newData) continue; //&& bc run can be false on cb-based stuff even w data coming //or rather w callbacks etc should be OR?
      if(!source->run() && !source->newData) continue; //&& bc run can be false on cb-based stuff even w data coming //or rather w callbacks etc should be OR?
      // ^ this could be fucked by letting through when run is positive but there's no data and just bs
      // source->run(); if(!source->newData) continue;

      // sourceData[source->id()]->set(source->get(), lastKeyframe);

      // renderer().controls->avg(cb, true); //ignores 0
      // renderer().controls->avg(cb, false); // doesnt ignore 0
      if(source->type() == "sacn") { //temp. use olad/afterglow etc for controls, not resolume...
        auto cb = Buffer(source->get(), ctrlChannels); // so, some issues here it seems, Buffer cb averages to 0, so source also appears to be 0?
        // renderer().f->setTarget(renderer().controls->get());
        renderer().f->setTarget(cb.get());
      } //oi dont forget iot ctrls
      //XXX def make blending strategy entirely configurable... for dev/testing, not the least...
      // auto pb = PixelBuffer(source->fieldSize(), source->fieldCount(), source->get().get(ctrlChannels));

      // if(onlyBlendSeparateSources) { //i mean add/sub etc from own source p retarded, tho possibly makes for interesting behavior/looks
      //   if(!newInput) { // this is first input since last keyframe for any source. obvs will get super random if lhs/rhs randomly swaps all the time heh
      //     sourceMix->set(pb); //also could just put first one as ignore 0 for same effect right
      //     newInput = true;
      //   } else {
      //     Renderer::mixBuffers(*sourceMix, pb, _iot->blendMode);
      //   }
      // } else {
      //   renderer().updateTarget(pb, _iot->blendMode, true); //p dumb
      // }
      newInput = true;
      logDMX(*source);
    }
  }

  void logDMX(Inputter& source) { //XXX make sure to set up sep per source on count end tho?
    //could also take an average instead of just exact at 10s...
    //+ sort able to request device dump its buffers and details to log, per mqtt...
    _iot->debug->logDMXStatus(source.get().get());
    _iot->debug->logFunctionChannels(source.get().get(), "SRC");
    _iot->debug->logFunctionChannels(renderer().controls->get(), "RAW"); // _iot->debug->logFunctionChannels(targetFunctions->get(), "MIX");
    _iot->debug->logFunctionChannels(renderer().f->ch, "FUN");
  }

  void mergeDMX() {
    bool firstActive = true;
    for(auto&& source: _input) {
      if(!source->receiving()) continue;
      if(firstActive) {
        pb->set(source->get());
        firstActive = false;
      } else Renderer::mixBuffers(*pb, source->getPB(), _iot->blendMode);
    }
    renderer().updateTarget(*pb);
    newInput = false;

    // // if(bothSources == active && bothSources == receivingData && bothSources->gotNewData) {
    //   // so need some sort of timeout for counting whether not just enabled, but actively receiving data
    //   renderer().updateTarget(*sourceMix, _iot->blendMode);
    //   for(auto i=0; i<sourceMix->length(); i++) //then clear mixbuf...
    //     // *sourceMix->get(i) = 20; // test, prob horribly crap
    //     *sourceMix->get(i) = 0; // test, prob horribly crap and causing that red residue? or hmm
    //   newInput = false;
    //   // uint8_t z[500] = {0};
    //   // sourceMix->fill(zero);
    // // }
  }

  void keepItMoving() { // start easing backup anim to avoid stutter... easiest/basic fade in a bg color while starting to dim
      // ev make configurable with multiple ways or dealing. and as time
    //first try just put black buffer as a (sub)target (with high attack)?
    //like, "target for source is now black", so with reg htp or no-0-avg wont actually head that way if other sources still active...
    //just avoid being stuck with a static image.
    //but how do that per source tho atm...
    //also, more effectively done as just request to fill color black with progressively higher alpha
  }

  void loop() {
    lwd.stamp(PIXTOL_SCHEDULER_ENTRY);
    handleDMX();
    uint32_t now = micros(), passed = now - lastKeyframe;
    if(newInput && passed > 1.0*keyFrameInterval) { // needs to be fancier but....
      lastKeyframe = now;
      mergeDMX(); //how more careful/avoid flashing from late frames causing kray?
      // just mix latest from all inputters together i suppose, even if results in some duplicates -
      // latest is latest, what else can ya do when rates not same/stable...
      // and waiting one interval means have time to actually interpolate to target and not get cut halfway there...
    } else if(passed > 2.0*keyFrameInterval) { //or when makes sense. maybe configurable...
      keepItMoving();
    }
    lwd.stamp(PIXTOL_RENDERER_FRAME);
    renderer().frame();
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
};
