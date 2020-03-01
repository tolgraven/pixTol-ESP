#pragma once

#include <Arduino.h>

class Named { // has a name and a type.
  public:
    Named(const String& id, const String& type = ""):
    _id(id), _type(type) {}
    const String& id()   const { return _id; }
    const String& type() const { return _type; }
    void   setId(const String& newId)   { _id = newId; }
    void setType(const String& newType) { _type = newType; }
  protected:
    String _id, _type; //or name? want for eg artnet, sacn, strip, shorter stuff not full desc
};
// setup with buffer(s) subdivided by elements of a specific size
class ChunkedContainer {  // (Buffer, but also others using them - RS)
  public:
    ChunkedContainer(uint8_t fieldSize, uint16_t fieldCount):
      _fieldSize(fieldSize), _fieldCount(fieldCount) {}
    uint8_t  fieldSize()   const { return _fieldSize;  }
    uint16_t fieldCount()  const { return _fieldCount; }
    void     setFieldSize (uint8_t  newFieldSize)  { _fieldSize  = newFieldSize;  }
    void     setFieldCount(uint16_t newFieldCount) { _fieldCount = newFieldCount; }
    uint16_t length()      const { return _fieldSize * _fieldCount; } // should be called totalSize or sizeInUnits or some shit dunno
  protected:
    uint8_t  _fieldSize;
    uint16_t _fieldCount;
};

// something expected to be run (usually continously, optionally by scheduling callback and setting a flag)
class Runnable {
  public:
    bool run() { if(ready()) _run(); }
    virtual bool ready() {}  // prob handle as _run and first check for activ() then pass down?
    virtual uint32_t microsTilReady() {}

    void setActiveState(bool state = true) { _active = state; }
    bool active() { return _active; }

    uint16_t maxHz = 0; // disabled by default
  private:
    virtual bool _run() {}
    bool _active = true;

  protected:
    // prob more generic terms. hz maybe ok. updates, not frames.
    uint16_t throughput = 0; // Just use micros per byte I guess? for strip ca 10us per subpixel + 50us pause end each.
    uint32_t timeLastRun = 0; // means last successfully run (by being propagated all the way up to base)
    uint32_t timeLastAttempt = 0; //
    uint16_t hz = 40; // generally fixed value for inputters, buffers get theirs from outputter with highest value, outputters dynamic value from size*rate? 0 = full tilt
    uint16_t currentHz = hz; // will have to rethink if stick with multiple inheritance so one object can be eg both inputter and outputter...  if inherit virtual then get stuck with same rates in and out which isnt necessarily at all what you'd want...
    uint32_t frames = 0; // counter for number of actual frames of data passed through stage
};


// From abov RenderStage def:

  // makes more sense having more of a skeleton interface then subclass to:
  // "Passthrough" that doesnt own data or do its own manipulation
  // "Real".
  // or these just always get created without buffers and need to be inited?
  // can circumvent having explicit patch objects by these always already
  // containing source(s) and output(s)
  // eg:
  // Artnet in:
  // source - packet buffer (hence dont own intermediate)
  // dests - Renderer *target buf (pixels) +
  //    tricky: Functions not associated w that buf but rather *current
  // MQTT in
  // source - specific subscribed (sub)topic
  // dests - something routing cmd val -> pos in some buf?
  // OR
  // MQTT in
  // source - everything reaching us
  // it then just passes shit on dumbly necessiting further yada
  //
  // UGH this shit is tricky

  // which speaks to having Functions as precisely how current RenderStage works
  // probably treated as a virtual outputter?
  // ie we have pre-set where data is and just continously run()
