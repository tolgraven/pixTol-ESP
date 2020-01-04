#pragma once
#include <vector>
#include "buffer.h"

// Base class in render pipeline, providing common API for working with stages.
// A stage contains one of more attached Buffers of fieldSize and fieldCount.
// and is continously run(), trying to fetch input or send data out.
class RenderStage { // should be called something else. dunno what
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
  public:
    RenderStage(const String& id, uint16_t numBytes): RenderStage(id, 1, numBytes) {}
    RenderStage(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      _id(id), _fieldSize(fieldSize), _fieldCount(fieldCount) {
      /* lg.dbg("Create RenderStage: " + id + ", bufferCount " + bufferCount); */
      for(auto i=0; i<bufferCount; i++) {
        buffers.emplace_back(new Buffer(id, fieldSize, fieldCount)); //XXX watch out, bufferCount has no effect atm
      }
    }
    virtual ~RenderStage() { buffers.clear(); }

    virtual bool run() { // only called from below when actually successful
      // here we need to dynamically adjust expected refresh rate.
      /* uint32_t now = micros(); */
      /* static uint32_t timeSinceAdjustedKeyframeInterval = 0; */
      /* if(_lastKeyframe > 0) timeSinceAdjustedKeyframeInterval += now - _lastKeyframe; //log says takes 3s between frame 1 and 2. Makes no sense... */
      /* _lastKeyframe = now; */
      /* _numKeyFrames++; */
      /* if(timeSinceAdjustedKeyframeInterval > keyFrameAdjustmentInterval) { */
      /*   static uint32_t numKeyFramesAtLastAdjustment = 0; */
      /*   setKeyFrameInterval((_numKeyFrames - numKeyFramesAtLastAdjustment) / timeSinceAdjustedKeyframeInterval); */
      /*   lg.f(__func__, Log::DEBUG, "frames: %u, frames last: %u, time since: %u, interval: %u\n", */
      /*       _numKeyFrames, numKeyFramesAtLastAdjustment, timeSinceAdjustedKeyframeInterval, getKeyFrameInterval()); */
      /*   numKeyFramesAtLastAdjustment = _numKeyFrames; */
      /*   timeSinceAdjustedKeyframeInterval = 0; */
      /* } */

      frames++;
      timeLastRun = micros();
      return true;
    } //to be called by inherited classes on successful run. which might be eh since so many instances....

    virtual bool ready() {
      return microsTilReady() == 0; //true;
    } //default in case not implementing these
    virtual uint16_t microsTilReady() {
      if(throughput == 0 && maxHz == 0) {
        return 0;
      } else {
        int32_t passed = micros() - timeLastRun;
        return max(1000000 / maxHz - passed, 0);
      }
    }

    void setActive(bool state = true) { _active = state; }
    bool active() { return _active; }

    const String& id(const String& newId = "") { if(newId.length()) _id = newId; return _id; }
    const String& type(const String& newType = "") { if(newType.length()) _type = newType; return _type; }
    uint8_t fieldSize() { return _fieldSize; }
    uint16_t fieldCount() { return _fieldCount; }
    virtual void setFieldSize(uint8_t newFieldSize) { //XXX sep set and get (slow, avoid virtual...) will need to actually propogate to Buffers etc...
      _fieldSize = newFieldSize; }
    virtual void setFieldCount(uint16_t newFieldCount) {
      _fieldCount = newFieldCount; }
    virtual uint16_t sizeInBytes() { return _fieldSize * _fieldCount; } //should also account for multiple buffers etc i guess..

    uint8_t bufferCount() { return buffers.size(); }
    virtual Buffer& get(uint8_t index = 0) __attribute__((deprecated)) { return buffer(index); } //XXX deprecaaterd
    virtual Buffer& buffer(uint8_t index = 0) {
      return *(buffers[index]);
    }

    virtual void setBuffer(Buffer& newBuffer, uint8_t index = 0) {
      buffer(index).setPtr(newBuffer); // should be ok since will delete current in buf
      /* delete buffers[index]; //urh p bad if it's someone elses heh... */
      /* buffers.clear(); */
      /* buffers.push_back(&newBuffer); //XXX most definitely FIXME */
    }
    void setGain(float gain) { Serial.printf("%.2f  ", gain); _gain = gain; }
    float getGain() { return _gain; } // counter for number of actual frames of data passed through stage

  protected:
    String _id, _type; //or name? want for eg artnet, sacn, strip, shorter stuff not full desc
    uint16_t throughput; // Just use micros per byte I guess? for strip ca 10us per subpixel + 50us pause end each.
    uint32_t timeLastRun; // means last successfully run (by being propagated all the way up to base)
    uint32_t timeLastAttempt; //
    uint16_t hz = 40; // generally fixed value for inputters, buffers get theirs from outputter with highest value, outputters dynamic value from size*rate? 0 = full tilt
    uint16_t currentHz = hz; // will have to rethink if stick with multiple inheritance so one object can be eg both inputter and outputter...  if inherit virtual then get stuck with same rates in and out which isnt necessarily at all what you'd want...
    uint16_t maxHz = 0;
    uint32_t frames = 0; // counter for number of actual frames of data passed through stage

    uint8_t _fieldSize;
    uint16_t _fieldCount;
    std::vector <Buffer*> buffers;
    float  _gain = 1.0;
    bool _active = true;
    uint16_t port; // UDP port, pin, interface id, whatever...
    uint8_t _priority = 0; //might be useful for all stages no? just had in inputter earlier
  private:
};


class Inputter: public RenderStage {
  public:
    Inputter(const String& id, uint16_t bufferSize = 512): // raw bytes = field is 1
      Inputter(id, 1, bufferSize) {}
    Inputter(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      RenderStage(id, fieldSize, fieldCount, bufferCount) {
      type("inputter");
    }
    /* int pixelsOffset; //temp before proper layout thing */
    // void setLayout() { } //get from cfg no? figure out

    bool newData = false;
    virtual bool run() override { //default, log time and affirmative. here's where we gotta timeout _isReceiving etc tho so either extra fn or add bool flag
      if(!active()) return false;
      newData = true;
      _isReceiving = true; //XXX needs to actually lapse somewhere somehow tho...
      return RenderStage::run(); // return true;
    }
    Buffer& buffer(uint8_t index = 0) override { // this isnt a good idea btw... way too implicit behavior
      // if(!_active) should return empty or rong or wha?
      newData = false; //maybe bad resetting, in case getting internally or w/e...
      return RenderStage::buffer(index);
    } //XXX check bounds

    void pin(bool state = true) { _isReceiving = _pinned = state; } // so can mix in eg static local bg color with no timeout...
    bool receiving() { return _isReceiving; }
  protected:
    bool dirty = true, _isReceiving = false, _pinned = false; //pinned inputters are "always fresh", even if not continously fed data.
};

// figure artnet/sacn/opc (more?) such common format could use this?
class NetworkIn: public Inputter {
  protected:
  // well this shouldnt be called run.
  bool run(uint16_t portId, uint8_t* data, uint16_t length = 0) {
    /* lg.f(__func__, Log::DEBUG, "NetworkIn %s: id %d\n", type().c_str(), portId); */
    if(!length || length > sizeInBytes())
      length = sizeInBytes();
    uint16_t bufferIndex = portId - startPort;
    if(bufferIndex < buffers.size()) {
      /* buffer(bufferIndex).set(data, length); */
      if(data != buffer(bufferIndex).get()) { // dont touch if already pointing to right place.
        /* buffer(bufferIndex).setCopy(data, length); //also prob just set ptr if Not? */
        // might be this is fucking off?
        buffer(bufferIndex).setPtr(data); //also prob just set ptr if not? tho idea is also gathering multiple universes into one buf...
                                          //but then that can be done downstream too? just easier if here knows whether all related data has arrived...
      } // remember here we already have one extra buffer + copy op
        // on top of wrapped inputter. So it's very safe to do stuff to this one.
      /* lg.f(__func__, Log::DEBUG, "NetworkIn run"); */
      return Inputter::run(); //propogate up, updates vars n shit
    } else {
      return false;
    }
  }
  public:
  NetworkIn(const String& id, uint16_t startPort, uint8_t numPorts, uint16_t pixels = 0):
    Inputter(id, 1, 512, numPorts), startPort(startPort) { type("NetworkIn"); } //tho some not so DMX-aligned
  //XXX fix compat with OPC. or skip run that through reg Inputter? hmm

  uint16_t startPort;
  int lastStatusCode;
  uint8_t lastSeq; //common enough?
};


class Outputter: public RenderStage {
  public:
  Outputter(const String& id, uint16_t bufferSize = 512):
    RenderStage(id, bufferSize) {} // raw bytes = field is 1
  Outputter(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
    RenderStage(id, fieldSize, fieldCount, bufferCount) { type("outputter"); }

  uint16_t maxFrameRate; // calculate from ledCount etc, use inherited common vars and methodds for that...
  uint16_t droppedFrames = 0; //makes sense? inputter cant really drop and know about it tho generally?
  bool run() {
    if(!_active) return false;
    return RenderStage::run();
  }
  protected:
  bool _mirror = false, _fold = false, _flip = false, _gammaCorrect = false; //these will go in Transform
  NeoGamma<NeoGammaTableMethod>* colorGamma;

  private:
  virtual uint16_t getIndexOfField(uint16_t position) { return position; }
  virtual uint16_t getFieldOfIndex(uint16_t field) { return field; }
};

class Generator: public Inputter {
  public:
    Generator(const String& id, uint16_t bufferSize = 1): // raw bytes = field is 1
      Inputter(id, bufferSize) {} // raw bytes = field is 1
    Generator(const String& id, uint8_t fieldSize, uint16_t fieldCount):
      Inputter(id, fieldSize, fieldCount) {
        type("Generator");
      }

    enum GeneratorType { Static, Animation, Playback };
    enum OnEnd { Stop, Loop, BackAndForth, Random }; //if Animation or, specifically, Playback. Or tbh up to whoever is requesting generator's services?

    bool run() {
      // so, do we reg to get called every x or just spam everything HF and let them handle on their own?
      // since real inputs cant be trusted to keep stable hz like
      // still have to account for that in general
      /* buffer().fill() */
      return Inputter::run(); // return true;
    }
  private:
    /* Buffer* input; //the commands to be translated to shit */
    /* BufferF* input; //the commands to be translated to shit */
};
