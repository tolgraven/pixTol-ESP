#pragma once

#include "renderstage.h"
#include "buffer.h"
#include "io/strip.h"
#include "functions.h"
#include "log.h"
#include "watchdog.h"

class Renderer {
  private:
    // std::map<String, PixelBuffer*> buffer; // out now pointed to strip, but/so why not just use s->buffer->interpolate like
    // Outputter* output = nullptr;
    uint32_t _keyFrameInterval, _lastKeyframe;
  public:
    PixelBuffer *current, *origin, *target; // *delta = nullptr;
    Buffer* controls = nullptr;
    Functions* f = nullptr;
    Strip* s = nullptr;

    Renderer(uint32_t keyFrameInterval, Outputter& o):
      _keyFrameInterval(keyFrameInterval), s(static_cast<Strip*>(&o)) {
      lg.dbg("Creating renderer");
      current = new PixelBuffer(o.fieldSize(), o.fieldCount());
      origin = new PixelBuffer(o.fieldSize(), o.fieldCount());
      target = new PixelBuffer(o.fieldSize(), o.fieldCount());
      // for(auto* pb: {current, origin, target}) {
      //   pb = new PixelBuffer(o.fieldSize(), o.fieldCount());
      // } // XXX why the fuck doesnt this work, fuck auto
      f = new Functions(keyFrameInterval, o);
      controls = new Buffer("Controls", 1, f->numChannels);
    }

  void updateTarget(PixelBuffer& mergeIn, uint8_t blendType = 0, bool mix = false) {
    lwd.stamp(PIXTOL_RENDERER_UPDATE_TARGET);
    origin->set(current->get()); //always start from current state...
    _lastKeyframe = micros();
    if(mix) mixBuffers(*target, mergeIn, blendType); //shouldnt actually be here - blending has been done, now chase new target...
    else target->set(mergeIn.get());
  }

  static void mixBuffers(PixelBuffer& lhs, PixelBuffer& rhs, uint8_t blendType = 0) {
    switch(blendType) { //control via mqtt as first step tho
      case 0: lhs.avg(rhs, true); break; //this needs split sources to really work, or off-pixels never get set
      case 1: lhs.avg(rhs, false); break;
      case 2: lhs.add(rhs); break;
      case 3: lhs.sub(rhs); break;
      case 4: lhs.htp(rhs); break;
      case 5: lhs.lotp(rhs, true); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
      case 6: lhs.lotp(rhs, false); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
      case 7: lhs.htp(rhs); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
    }
  }

  void frame() {
    lwd.stamp(PIXTOL_RENDERER_FRAME);
    float progress = (micros() - _lastKeyframe) / (float)_keyFrameInterval; //remember to adjust stuff to interpolation can overshoot...
    if(progress > 1.0) progress = (int)progress + 1.0f - progress; //make go back and forth instead! so 2.5 = 0.5...
    //should do further ultra slomo sorta, explore mini interpo more...

    current->blendUsingEnvelope(*origin, *target, f->e, progress);
    // current->interpolate(origin, target, progress, gammaPtr);

    // s->setBuffer(*current); //test setting per-pixel in buffer for now
    for(auto pixel=0; pixel < s->fieldCount(); pixel++) { //guess this needed for rotate, brightness etc to work. why tho??
      uint8_t* data = current->get(pixel * s->fieldSize());
      RgbwColor color = RgbwColor(data[0], data[1], data[2], data[3]);
      s->setPixelColor(pixel, color);
    }
    f->update(progress);
    s->show(); // s->run();
  }
};
