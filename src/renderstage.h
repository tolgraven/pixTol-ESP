#pragma once
#include <vector>
#include "buffer.h"


class RenderStage {
  public:
    RenderStage(const String& id, uint16_t numBytes):
      RenderStage(id, 1, numBytes) {}
    RenderStage(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      _id(id), _fieldSize(fieldSize), _fieldCount(fieldCount), _bufferCount(bufferCount) {
      for(auto i=0; i < _bufferCount; i++) {
        buffers.push_back(new Buffer(_id, _fieldSize, _fieldCount)); // make buffer id like renderstageID_buffer_size
      }
    }
    RenderStage(const String& id, Buffer* buffer) {
      _id = id;
      _fieldSize = buffer->fieldSize();
      _fieldCount = buffer->fieldCount();
      buffers.push_back(buffer);
    }
    virtual ~RenderStage() { buffers.clear(); }

    virtual void setup() {};
    virtual bool run() = 0;
    virtual bool ready() { return true; } //default in case not implementing these
    virtual uint16_t microsTilReady() { return 0; }

    void setActive(bool state = true) { _active = state; } //temp //= 0;  // idx -1 = all
    bool active() { return _active; }

    const String& id() const { return _id; }
    auto fieldSize(uint8_t newFieldSize = 0) { //XXX will need to actually propogate to Buffers etc...
      if(newFieldSize) _fieldSize = newFieldSize;
      return _fieldSize;
    }
    auto fieldCount(uint16_t newFieldCount = 0) {
      if(newFieldCount) _fieldCount = newFieldCount;
      return _fieldCount;
    }
    virtual uint16_t length() { return _fieldSize * _fieldCount; }
    // virtual Buffer& get(uint8_t index = 0, uint16_t offset = 0) { return *(buffers[index] + offset); } //XXX check bounds
    virtual Buffer& get(uint8_t index = 0) { return *(buffers[index]); } //XXX check bounds
    // virtual uint8_t* get(uint8_t index = 0) { return buffers[index]->get(); } //XXX check bounds
    virtual RenderStage& setBuffer(Buffer& buffer, uint8_t index = 0) {
      buffers[index]->set(buffer);
    }

    uint16_t hz = 40; // generally fixed value for inputters, buffers get theirs from outputter with highest value, outputters dynamic value from size*rate? 0 = full tilt
    uint16_t currentHz = hz; // will have to rethink if stick with multiple inheritance so one object can be eg both inputter and outputter...  if inherit virtual then get stuck with same rates in and out which isnt necessarily at all what you'd want...
  protected:
    uint16_t throughput; // Just use micros per byte I guess? for strip ca 10us per subpixel + 50us pause end each.
    uint32_t timeLastRun;
    String _id;
    uint8_t _fieldSize;
    uint16_t _fieldCount;
    uint8_t _bufferCount;
    std::vector<Buffer*> buffers; //so index, bufcount etc just get from here...
    bool _active = true;
    enum InterfaceType { NET, GPIO, SPI }; //or something. tho should be able to support several(?), or then artnet - ethernet, dmx - serial, different modules actually...
    uint16_t port; // UDP port, pin, interface id, whatever...
  private:
};


class Inputter: public RenderStage {
  public:
    Inputter(const String& id, uint16_t bufferSize = 512): // raw bytes = field is 1
      RenderStage(id, bufferSize) {}
    Inputter(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      RenderStage(id, fieldSize, fieldCount, bufferCount) {}

    bool newData = false;
    bool run() {
      newData = true;
      timeLastRun = micros();
      return true;
    }
    Buffer& get(uint8_t index = 0) {
      // if(!_active) should return empty or rong or wha?
      newData = false; //maybe bad resetting, in case getting internally or w/e...
      return RenderStage::get(index);
    } //XXX check bounds
    PixelBuffer& getPB() { //only one yeah? if can stitch shit like that easy nuff... ref bit shit since might not be a pixel kinda inputter = ???... temp
      //yada lada create off of layout...
      // ideally create on spot a thin copy of our Buffer, with only offset etc adjusted...
      // pb->set(get(), 0, pixelsOffset); //like here, we using it form inside...
      pb->set(*buffers[0], 0, pixelsOffset); //just make sure pointer is fresh in case main Buffer moved...
      return *pb;
    } //XXX TEMP SHIT

    void pin(bool state = true) { // so can mix in eg static local bg color with no timeout...
      _isReceiving = _pinned = state;
    }
    bool receiving() { return _isReceiving; }
  protected:
    // XXX something, layout? seems maybe silly/tricky to put actual merged/split/whatever
    // Buffers/PixelBuffers etc etc, but still, should know at this stage wtf is in incoming data...
    // THOUGHT: ditch multiple buffers and just use one?
    // thought to not copy data needlessly could require Buffer supporting multiple pointers for data
    // which might make sense anyways, as long as we can encapsulate / properly move away from ever
    // touching raw uint8_t*...
    // OR never even think of "original" data but gut it as it comes...
    // (no "buffer0")
    bool _isReceiving = false;
    bool _pinned = false; //pinned inputters are "always fresh", even if not continously fed data.
    bool dirty = true;
};


class Generator: public RenderStage {
  public:
    Generator(const String& id, uint16_t bufferSize = 512): // raw bytes = field is 1
      RenderStage(id, bufferSize) {} // raw bytes = field is 1
    Generator(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      RenderStage(id, fieldSize, fieldCount)
    {}

    virtual uint8_t* get(bool copy = false) = 0; //buffer?

  private:
    uint8_t priority = 0;
    bool isDirty = true;
};


class Outputter: public RenderStage {
  public:
    Outputter(const String& id, uint16_t bufferSize = 512):
      RenderStage(id, bufferSize) {} // raw bytes = field is 1
    Outputter(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      RenderStage(id, fieldSize, fieldCount, bufferCount) {}

    uint16_t droppedFrames = 0; //makes sense? inputter cant really drop and know about it tho generally?

  private:
    virtual uint16_t getIndexOfField(uint16_t position) { return position; }
    virtual uint16_t getFieldOfIndex(uint16_t field) { return field; }
};
