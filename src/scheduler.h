#pragma once

#include <vector>
#include <memory>

#include "renderer.h"
#include "buffer.h"
#include "io/strip.h"
#include "io/sacn.h"
#include "io/artnet.h"
#include "io/opc.h"
#include "io/unit.h"
#include "functions.h"
#include "device.h"
#include "watchdog.h"

// freeze buffer / background thing, run constant at lowest prio whether static or
// last frame from w/e source, could also interpolate between a few frames or have basic wave animation etc
// as higher layers of input can be nuking stuff from this (overwriting, or non-HTP taking away)
class Scheduler {
  String _id;
  Renderer* _renderer;
  std::vector<std::unique_ptr<Renderer>> _render;
  std::vector<std::unique_ptr<Inputter>> _input;
  std::vector<std::unique_ptr<Outputter>> _output;
  // also reg all components I guess
  // std::vector<Patch*> _patch;
  bool everInput = false, newInput = false;
  String controlsSource = "artnet"; // "" // should just pick up whichever is actively sending (XXX set up "dont ignore 0 if prev pos input from src in session") */
  Device* _device = nullptr;

  uint32_t start, keyFrameInterval, lastKeyFrame; // target interval between keyframes, abs time (micros) last frame read

  public:
  Scheduler(const String& id, Device& device):
    _id(id), _device(&device) {

    int startUni = 2;
    addIn(new ArtnetInput(id + " ArtNet", startUni, 1));
    addIn(new sAcnInput(id + " sACN", startUni, 1));
    /* _input.emplace_back(new OPC()); */
    lg.dbg("Done add inputs");

    addOut(new Strip("Strip 1", 4, 120));
    // static_cast<Strip&>(output(0)).mirror(cfg.mirror.get()).fold(cfg.fold.get()); // s->flip(false); //no need for now as no setting for it //cfg.setFlipped.get();
    lg.dbg("Done add outputs");

    keyFrameInterval = MILLION / 40; //assumption. should be adaptive tho... if actual continously different

    _renderer = new Renderer("Renderer", keyFrameInterval, output(0).buffer()); //doesnt use the buffer, it becomes model. tho current might as well be it meh dunno
    renderer().getDestination().setPtr(output(0).buffer()); // in turn directly pointed to Strip buffer (for DMA anyways, handled internally anyways)
    uint8_t order[] = {1, 0, 2, 3}; // should be set/retrieved from Strip tho...
    renderer().getDestination().setDestinationSubFieldOrder(order); //tho last one is a bit, heh.
    renderer().getDestination().setDithering(true);
    lg.dbg("Done create renderer");

    lg.f(__func__, Log::DEBUG, "Buffer ptrs: input0 %p,  output0 %p,  renderer c buf/ptr: %p %p\n",
        input(0).buffer().get(), output(0).buffer().get(), &renderer().getDestination(), renderer().getDestination().get());
    start = micros();
    lastKeyFrame = start; //idunno whether this or 0 hmm
  }

  Renderer& renderer() { return *_renderer; }

  Inputter& input(uint8_t index) { return *_input[index].get(); } //XXX bounds
  /* Inputter& input(const String& type) { */
  /*   // something something copy operator. */
  /*   for(auto in: _input) if(type == in.get()->type()) return *in.get(); */
  /* } //watch for if got multiple obj of same type tho... but only likely on outputter end? or physical ports maybe hmm... */
  /* ^^ eh danger. either type or id, choose... */
  Outputter& output(uint8_t index) { return *_output[index].get(); }
  /* Outputter& output(const String& id) { */
  /*   for(auto out: _output) if(id == out.get()->id()) return *out.get(); */
  /* } */
  // guess below but taking shader_ptr or eh dunno
  void addIn(Inputter* in) { _input.emplace_back(in); }
  void addOut(Outputter* out) { _output.emplace_back(out); }

  void handleInput() {
    lwd.stamp(PIXTOL_SCHEDULER_INPUT_DMX);

    for(auto&& source: _input) {
      if(!source->run() && !source->newData) {  // for artnet run -< handler -> callback if opdmx so newdata check redundant for single or disconnected universes. but anyways.
        continue;                               // tho curr those return true by default so this fixes run success but no data
        /* if(source->timeLastRun > yada) source->setActive(false); // this what source needs to calc in run */
      }

      if(controlsSource == source->type()) { //pref matches
        renderer().f->update(source->buffer().get()); //inputter buffer() currently resets newData which seems dangerous... but makes above thing not double up, for now
        /* lg.f(__func__, Log::DEBUG, "update controls"); */
      } else if(!controlsSource.length()) { //no preference on what to use
        renderer().controls->avg(source->buffer(), false); //false
        renderer().f->update(renderer().controls->get());
      }

      newInput = true;
      logDMX(*source);
    }
    /* keyFrameInterval = ...read from sources... */
  }

  void logDMX(Inputter& source) { //XXX make sure to set up sep per source on count end tho?
    //could also take an average instead of just exact at 10s...
    //+ sort able to request device dump its buffers and details to log, per mqtt...
    _device->debug->logDMXStatus(renderer().f->b->get(), "SOURCE");
    /* _device->debug->logDMXStatus(output(0).buffer().get(), "OUTPUT"); */
  }

  void mergeInput() { // this is sep from handleInput orig for when multiple sources (no diff tho how it's curr used)
    bool blank = true;
    for(auto&& source: _input) {
      if(!source->receiving()) continue; //XXX doesnt do anything active yet heh
      if(!everInput) everInput = true; //flag that any of our inputters have ever gotten shiet
      Buffer offsetBuffer = Buffer(source->buffer(), output(0).fieldCount(), renderer().f->numChannels, false);
      offsetBuffer.setFieldSize(output(0).fieldSize());

      if(blank) { //mixed buffer needs to be reset for new frame
        renderer().updateTarget(offsetBuffer);
        blank = false;
      } else {
        int blendMode = 0; //used to come from iot but decoupled
        renderer().updateTarget(offsetBuffer, blendMode, true);
      }
    }
    newInput = false;
  }

  void keepItMoving(uint32_t passed) { // start easing backup anim to avoid stutter... easiest/basic fade in a bg color while starting to dim
    Buffer* fBuffer = new Buffer("FunctionFill", 1, 12);
    fBuffer->get()[0] = 155;
    fBuffer->get()[3] = 100; //no longer doing env in frame() so just keeps from having correct target...
    fBuffer->get()[4] = 127;
    renderer().f->update(fBuffer->get()); //this should work unless something is super broken. Although the a proper patching source -> f-buffer / pixelbuffer and similar still has to be done...
    delete fBuffer;

    Color fillColor = Color(255, 140, 120, 75); /* soon move this out to keyframe gen */
    Buffer* fillBuffer = new Buffer("ColorFill", output(0).fieldSize(), output(0).fieldCount());
    fillBuffer->fill(fillColor);
    renderer().updateTarget(*fillBuffer);
    delete fillBuffer;
    // ev make configurable with multiple ways or dealing. and as time
    //first try just put black buffer as a (sub)target (with high attack)?
    //like, "target for source is now black", so with reg htp or no-0-avg wont actually head that way if other sources still active...
    //just avoid being stuck with a static image.
    //but how do that per source tho atm...
    //also, more effectively done as just request to fill color black with progressively higher alpha
  }

  void loop() {
    lwd.stamp(PIXTOL_SCHEDULER_ENTRY);
    handleInput();
    static bool inFallbackMode = false;
    uint32_t now = micros(), passed = now - lastKeyFrame;
    float progressMultiplier = 0;
    if(newInput) { //XXX temp, still moving towards latest keyframe so just spit out interpol
    /* if(newInput && passed > keyFrameInterval) { //XXX temp, still moving towards latest keyframe so just spit out interpol */
      // before also had check on how long since last frame. some kinda throttling makes sense
      // or we get no time for dithering. but needs to be less fuckoff than "one (1) expected keyframe interval"
      lastKeyFrame = now;
      inFallbackMode = false;
      mergeInput();
    }
    // prob makes sense going straight for a new frame no? esp since handle + merge wouldve taken extra long with data coming in!
    if(passed > 5 * 40 * keyFrameInterval) { //or when makes sense. maybe configurable...
      if(!inFallbackMode) {
        lg.dbg("No input for 5 seconds, running fallback...");
        inFallbackMode = true;
        keepItMoving(passed); //XXX only once so doesnt takee over like dmx...
      }
      progressMultiplier = 0.001;
    }
    renderer().frame(progressMultiplier);
    lwd.stamp(PIXTOL_RENDERER_FRAME);
    /* lg.f("Scheduler", Log::DEBUG, "dimmer strip/buf: %.2f %.2f\n", output(0).getGain(), renderer().current->getGain()); */
    output(0).run(); //output has been pointed to use Renderer->current. Guess this way we get unecessary flushes but
    lg.fEvery(1000, 1, __func__, Log::DEBUG, "%u\n", renderer()._numFrames);
  }
};
