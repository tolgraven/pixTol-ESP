#pragma once

#include "renderstage.h"
#include "buffer.h"
#include "io/strip.h"
#include "functions.h"

class Renderer {
  private:
    // std::map<String, PixelBuffer*> buffer; // out now pointed to strip, but/so why not just use s->buffer->interpolate like
    PixelBuffer *current, *origin, *target; // *delta = nullptr;
    Buffer* controls = nullptr;
    Functions* f = nullptr;
  public:
    Renderer() {
      f = new Functions(MILLION / iot->cfg->dmxHz.get(), s);
      controls = new Buffer("Strip fn target", 1, f->numChannels);

      target  = new PixelBuffer(4, 125);
      controls = new Buffer("Controls", 1, 12);
      output = new Strip("StripInRenderer", LEDS::RGBW, 125);
    }

    void update() { // runs all the fucking time, sometimes results in new inputs, otherwise keeps running existing interpolation til next keyframe
      if(source->run() && source->newData) { //new keyframe
        origin->set(current->get()); //dont jump on earlier than expected frames
        // delta = current->difference(target);
      } else {
        // current->blendWith(target, blendBaseline);
        // OR
        // current->add(delta);
      }
    }
};
