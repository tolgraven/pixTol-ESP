#pragma once

#include "modulator.h"
/* #include "buffer.h" */

class RenderStage {
  public:
    // RenderStage() {}
    RenderStage(const String& id, uint16_t size):
      name(id), fieldSize(1), fieldCount(size), dataLength(size) {}
    RenderStage(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      name(id), fieldSize(fieldSize), fieldCount(fieldCount),
      dataLength(fieldSize * fieldCount), bufferCount(bufferCount)
    {}
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
    // virtual void setActive(bool state = true, int8_t bufferIndex = -1) { //= 0;  // idx -1 = all
    //   isActive = state; //temp
    // }
    // virtual void setActive(bool state = true, int8_t bufferIndex = -1) = 0;  // idx -1 = all

    String name;
    uint8_t fieldSize;
    uint16_t fieldCount;
    uint16_t dataLength;
    uint8_t bufferCount; // multiple streams must be supported - sep universes etc
    // std::vector<uint8_t*> data;
    bool isActive = true;
    // std::vector<bool> isActive; // yeah this kinda data better handled by Buffer?
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
    Inputter(const String& id, uint16_t bufferSize = 512): // raw bytes = field is 1
      RenderStage(id, bufferSize) {} // raw bytes = field is 1
    Inputter(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      RenderStage(id, fieldSize, fieldCount, bufferCount)
    {}

    // virtual void run() { read(); }
    virtual bool canRead() = 0;
    virtual void read() = 0;

    virtual bool addDestination(uint8_t* data, uint16_t length, uint16_t offset) = 0;
    // problem since usually want to merge + split data (eg 12 fn chs, rest pixel data, maybe also more universes of pixel data)

  private:
    uint8_t priority = 0;
    // virtual uint16_t getIndexOfField(uint16_t position) = 0; //{ return position; }
    // virtual uint16_t getFieldOfIndex(uint16_t field) = 0; // return field; }
};


class Outputter: public RenderStage {
  public:
    Outputter(const String& id, uint16_t bufferSize = 512):
      RenderStage(id, bufferSize) {} // raw bytes = field is 1
    Outputter(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      RenderStage(id, fieldSize, fieldCount, bufferCount) {}

    // virtual void run() { emit(); } // would have pointer to data and frame len already set
    virtual bool canEmit() = 0; // always check before trying to emit...
    virtual void emit(uint8_t* data, uint16_t length) = 0;

  protected:
    uint16_t droppedFrames = 0;

  private:
    virtual uint16_t getIndexOfField(uint16_t position) { return position; }
    virtual uint16_t getFieldOfIndex(uint16_t field) { return field; }

};

