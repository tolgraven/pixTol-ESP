#pragma once

#include "modulator.h"
/* #include "buffer.h" */

class RenderStage {
  public:
    // RenderStage() {}
    RenderStage(const String& id, uint16_t size):
      name(id), fieldSize(1), fieldCount(size), dataLength(size) {}
    RenderStage(const String& id, uint8_t fieldSize, uint16_t fieldCount):
      name(id), fieldSize(fieldSize), fieldCount(fieldCount), dataLength(fieldSize * fieldCount) {}
    ~RenderStage() {
      // for(Modulator<uint8_t>* mod: mods) { delete mod; } // doez I need dis?
    }

    RenderStage& addModulator(Modulator<uint8_t>* mod) {
      // mods.push_back(mod);
      return *this;
    }
    // void applyModulators() {
    //   for(Modulator<uint8_t>* mod: mods) {
    //     applyModulator(mod);
    //   }
    // }
    // virtual void run() = 0;
    // virtual void enable() = 0;
    // virtual void disable() = 0;

    String name;
    uint8_t fieldSize;
    uint16_t fieldCount;
    uint16_t dataLength;
    // uint8_t* data; // should have in each? or make use of existing eg artnet.getDmxDFrame(),

    uint16_t throughputRate; // some sort of measure of throughput, so can scale accordingly
    uint16_t hz = 40; // generally fixed value for inputters, buffers get theirs from outputter with highest value, outputters dynamic value from size*rate...
    // will have to rethink if stick with multiple inheritance so one object can be eg both inputter and outputter...
    // if inherit virtual then get stuck with same rates in and out which isnt necessarily at all what you'd want...
  protected:

  private:
    // virtual void applyModulator(Modulator<uint8_t>* mod) = 0; //how pass ref instead?
    //
    // std::vector<Modulator<uint8_t>*> mods;
};


class Inputter: public RenderStage {
  public:
    Inputter(const String& id, uint16_t bufferSize): // raw bytes = field is 1
      RenderStage(id, bufferSize) {} // raw bytes = field is 1
    Inputter(const String& id, uint8_t fieldSize, uint16_t fieldCount):
      RenderStage(id, fieldSize, fieldCount) {}

    // virtual void run() { read(); }
    virtual bool canRead() = 0;
    virtual void read() = 0;

    virtual bool addDestination(uint8_t* data, uint16_t length, uint16_t offset) = 0;
    // problem since usually want to merge + split data (eg 12 fn chs, rest pixel data, maybe also more universes of pixel data)

    // virtual void enable(uint8_t port); // can use like Inputter::enable() for when inherit both Inputter and Outputter right?
    // virtual void disable(uint8_t port); // so these go in renderstage or? hmm
    //
  private:
    uint8_t priority = 0;
    // virtual uint16_t getIndexOfField(uint16_t position) = 0; //{ return position; }
    // virtual uint16_t getFieldOfIndex(uint16_t field) = 0; // return field; }

};


class Outputter: public RenderStage {
  public:
    Outputter(const String& id, uint16_t bufferSize):
      RenderStage(id, bufferSize) {} // raw bytes = field is 1
    Outputter(const String& id, uint8_t fieldSize, uint16_t fieldCount):
      RenderStage(id, fieldSize, fieldCount) {}

    // virtual void run() { emit(); } // would have pointer to data and frame len already set
    virtual bool canEmit() = 0; // always check before trying to emit...
    virtual void emit(uint8_t* data, uint16_t length) = 0;

  protected:
    uint16_t droppedFrames = 0;

  private:
    // virtual uint16_t getIndexOfField(uint16_t position) = 0; //{ return position; }
    virtual uint16_t getIndexOfField(uint16_t position) { return position; }
    // virtual uint16_t getFieldOfIndex(uint16_t field) = 0; // return field; }
    virtual uint16_t getFieldOfIndex(uint16_t field) { return field; }

};

