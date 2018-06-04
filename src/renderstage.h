#pragma once

#include "modulator.h"

class RenderStage {
  public:
    void addModulator(Modulator mod) {
      
    }
    void applyModulators() {
      for(mod: mods) {
        applyModulator(mod);
      }
    }

  private:
    void applyModulator(Modulator mod);

    std::vector<Modulator> mods;
    String name;
    uint16_t size;
    uint16_t rate; // some sort of measure of throughput, so can scale accordingly
    uint16_t hz = 40; // generally fixed value for inputters, buffers get theirs from outputter with highest value, outputters dynamic value from size*rate...
};
