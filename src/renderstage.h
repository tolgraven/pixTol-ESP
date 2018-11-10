#pragma once
#include <vector>
#include "buffer.h"

// enum LEDS: uint8_t {
//   Invalid = 0,
//   Single = 1, RED = 1, GREEN = 1, BLUE = 1, WHITE = 1,
//   RGB = 3, RGBW = 4, RGBWAU = 6
// };


class RenderStage {
  public:
    RenderStage(const String& id, uint16_t size): //if buffer not RenderStage, and RenderStage always contains Buffer*(s) of some sort, should set/get that info by way of buffer, rather?
      RenderStage(id, 1, size) { }
    RenderStage(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      _id(id), _fieldSize(fieldSize), _fieldCount(fieldCount) {
      buffers.push_back(new Buffer(_id, _fieldSize, _fieldCount)); // make buffer id like renderstageID_buffer_size
    }
    RenderStage(const String& id, Buffer* buffer) {
      _id = id;
      _fieldSize = buffer->fieldSize();
      _fieldCount = buffer->fieldCount();
      buffers.push_back(buffer);

    }
    virtual ~RenderStage() {}

    virtual void setup() {};
    virtual bool run() = 0;
    virtual bool ready() = 0;
    virtual uint16_t microsTilReady() = 0;

    void setActive(bool state = true) { _active = state; } //temp //= 0;  // idx -1 = all
    bool active() { return _active; }

    const String& id() const { return _id; }
    uint8_t fieldSize(uint8_t newFieldSize = 0) {
      if(newFieldSize) _fieldSize = newFieldSize;
      return _fieldSize;
    }
    uint16_t fieldCount(uint16_t newFieldCount = 0) {
      if(newFieldCount) _fieldCount = newFieldCount;
      return _fieldCount;
    }
    uint16_t length() { return _fieldSize * _fieldCount; }
    Buffer& get(uint8_t index = 0) { return *buffers[index]; } //XXX check bounds

    uint16_t hz = 40; // generally fixed value for inputters, buffers get theirs from outputter with highest value, outputters dynamic value from size*rate? 0 = full tilt
    uint16_t currentHz = hz; // will have to rethink if stick with multiple inheritance so one object can be eg both inputter and outputter...  if inherit virtual then get stuck with same rates in and out which isnt necessarily at all what you'd want...
  protected:
    uint16_t throughput; // Just use micros per byte I guess? for strip ca 10us per subpixel + 50us pause end each.
    uint32_t timeLastRun;
    String _id;
    uint8_t _fieldSize;
    uint16_t _fieldCount;
    std::vector<Buffer*> buffers; //so index, bufcount etc just get from here...
    bool _active = true;
    enum InterfaceType { NET, GPIO, SPI }; //or something. tho should be able to support several(?), or then artnet - ethernet, dmx - serial, different modules actually...
    uint16_t port; // UDP port, pin, interface id, whatever...
  private:
};


class Inputter: public RenderStage { //so thinking as this dealing with lotsa callbacks, dont inherit just pass fns?
  // or still inherit, just keep callbacks outside... or static. bc i mean urgh just decide
  public:
    Inputter(const String& id, uint16_t bufferSize = 512): // raw bytes = field is 1
      RenderStage(id, bufferSize) {}
    Inputter(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      RenderStage(id, fieldSize, fieldCount) {}

  private:
    uint8_t priority = 0;
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
