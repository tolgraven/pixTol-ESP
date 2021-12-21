#pragma once
#include <vector>
#include <stdexcept>
#include <Print.h>

#include "smooth/core/ipc/TaskEventQueue.h"
#include "smooth/core/ipc/IEventListener.h"

#include "buffer.h"
#include "base.h"
#include "task.h"

namespace tol {
  
enum class Patching { IN, OUT, INCopy, OUTCopy };
enum class PatchType { Controls, Pixels };

template<Patching dir, auto dest = PatchType::Pixels>
struct BufferPatch {
  BufferPatch() = default;
  BufferPatch(const Buffer& buffer, int i): buffer(buffer), i(i) {}
  // BufferPatch(Buffer& buffer, int i): buffer(&buffer), i(i) {}
  // BufferPatch& operator=(BufferPatch& rhs) {
  //   buffer = rhs.buffer; i = rhs.i; return *this;
  // }
  BufferPatch& operator=(const BufferPatch& rhs) = default;
  // Buffer* buffer = nullptr;
  Buffer buffer = Buffer(); // XXX copy needed since eg slicing in ArtnetIn, but for most stuff really inefficient yeah...
  // Buffer& buffer;
  int i = 0;
};

using PatchIn = BufferPatch<Patching::IN>;
using PatchOut = BufferPatch<Patching::OUT>;
using PatchControls = BufferPatch<Patching::IN, PatchType::Controls>;
using namespace smooth::core;
// Base class in render pipeline, providing common API for working with stages.
// A stage contains one of more attached Buffers of fieldSize and fieldCount.
// and is continously run(), trying to fetch input or send output.
class RenderStage: public ChunkedContainer, public Runnable { // should be called something else. dunno what
  public:
    RenderStage(const String& id, uint16_t numBytes): RenderStage(id, 1, numBytes) {}
    RenderStage(const String& id, uint8_t fieldSize, uint16_t fieldCount,
                uint8_t bufferCount = 1, uint16_t portIndex = 0);
    virtual ~RenderStage() { _buffers.clear(); }
    virtual void init() {}

    size_t printTo(Print& p) const override;
    void sendDiagnostic(const String s) {} // impl as Stream? art has pollreply freestle text, or  opt for using whatever output...

    void setSubFieldOrder(const uint8_t subFields[]) override {
      for(auto b: buffers()) b->setSubFieldOrder(subFields);
    }

    virtual uint32_t sizeInBytes() const;

    virtual Buffer& buffer(uint8_t index = 0) const;
    Buffer& operator[](uint16_t index) { return buffer(index); } //dunno but bit convinient
    const std::vector<std::shared_ptr<Buffer>>& buffers() const { return _buffers; } // maybe add range or filter tho

    void setBuffer(Buffer& pointTo, uint8_t index = 0) {
      _buffers.at(index) = std::shared_ptr<Buffer>(&pointTo);
      // XXX or rather repoint the ptr? again issue w this semi-managed shit is it's all-in or nothing...
      // tho as Buffers themselves still have notion of raw data ptr ownership shouldnt result in anything hmm
      // _buffers.at(index).assign(&pointTo);
    }
    void setGain(float gain) {  _gain = gain; }
    float getGain() const { return _gain; }

    void setPortId(uint16_t port) { _portId = port; }
    uint16_t portId() { return _portId; }

    bool dirty() const;

  protected:
    std::vector<std::shared_ptr<Buffer>> _buffers; // kow I've discoved multiple multiple times why use ptrs inside.
    float  _gain = 1.0; //applies to buffers within unless overridden.
    uint16_t _portId; // to start, only continous possible...   UDP port, pin, interface id, whatever...
    uint8_t _priority = 0; //might be useful for all stages no? just had in inputter earlier
  private:
};

class Router: public RenderStage {
  // would need to support input buffers (likely those created by default...)
  // and further buffers (for routing controls sep from data etc)
  // and would be the one to actually publish...

};

class Inputter: public RenderStage {
  // std::shared_ptr<TaskEventQueue<Buffer>> sendQueue; // POSTING. pass in ctor so can post specific and not to everyone
  // std::shared_ptr<SubscribingTaskEventQueue<Buffer>> queue; // POSTING. pass in ctor so can post specific and not to everyone
  public:
    Inputter(const String& id, uint16_t bufferSize = 512): Inputter(id, 1, bufferSize) {} // raw bytes = field is 1
    Inputter(const String& id, uint8_t fieldSize, uint16_t fieldCount,
             uint8_t bufferCount = 1, uint16_t portIndex = 0):
      RenderStage(id, fieldSize, fieldCount, bufferCount, portIndex) {
      setType("inputter"); // just use RTTI...
    }

    bool onData(uint16_t index, uint16_t length, uint8_t* data, bool flush = true);
    int controlDataLength = 12;

    virtual void onCommand() {}
};

class NetworkIn: public Inputter { // figure artnet/sacn/opc (more?) such common format could use this?
  public:
  NetworkIn(const String& id, uint16_t startPort, uint8_t numPorts, uint16_t pixels = 0):
    Inputter(id, 1, 512, numPorts, startPort) {
      setType("NetworkIn");
    } //tho some not so DMX-aligned

  protected:
  // bool isFrameSynced = false; // dunno if others, but Artnet lots timecode + flush stuff.
  int lastStatusCode;
  // uint8_t lastSeq; //common enough?
};


class Outputter: public RenderStage, public PureEvtTask, public Sub<PatchOut> {
  public:
  Outputter(const String& id, uint16_t bufferSize = 512):
    Outputter(id, 1, bufferSize) {} // raw bytes = field is 1
  Outputter(const String& id, uint8_t fieldSize, uint16_t fieldCount,
            uint8_t bufferCount = 1, uint16_t portIndex = 0):
    RenderStage(id, fieldSize, fieldCount, bufferCount, portIndex),
    PureEvtTask((id + " event runner").c_str(), 2),
    Sub<PatchOut>(this, 4) {
      setType("outputter");
      start();
    }

  virtual bool flush(int b = -1) { return false; }
  private:
};



class Generator: public Inputter {
  public:
    Generator(const String& id, uint16_t bufferSize = 1): // raw bytes = field is 1
      Generator(id, 1, bufferSize, 1) {} // raw bytes = field is 1
    Generator(const String& id, const RenderStage& model):
      Generator(id, model.fieldSize(), model.fieldCount(), model.buffers().size()) {}
    Generator(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1):
      Inputter(id, fieldSize, fieldCount, bufferCount) {
        setType("Generator");
      }

    enum GeneratorType { Static, Animation, Playback };
    enum OnEnd { Stop, Loop, BackAndForth, Random }; //if Animation or, specifically, Playback. Or tbh up to whoever is requesting generator's services?

    // std::vector<Animator> anims; // the one that will actually update buffers.
    // bool _run() override {
    //   return false;
    // }
  private:
};


  //void keepItMoving(uint32_t passed) { // start easing backup anim to avoid stutter... easiest/basic fade in a bg color while starting to dim
  //  auto fBuffer = Buffer("FunctionFill", 1, 12);
  //  fBuffer[0] = 155;
  //  fBuffer[3] = 100; //no longer doing env in frame() so just keeps from having correct target...
  //  fBuffer[4] = 127;
  //  renderer().f->update(fBuffer.get()); //this should work unless something is super broken. Although the a proper patching source -> f-buffer / pixelbuffer and similar still has to be done...

  //  Color fillColor = Color(255, 140, 120, 75); /* soon move this out to keyframe gen */
  //  auto fillBuffer = Buffer(output(0).buffer()); //will copy existing structure but also data...
  //  fillBuffer.fill(fillColor);
  //  renderer().updateTarget(fillBuffer);
  //  // ev make configurable with multiple ways or dealing. and as time first try just put black buffer as a (sub)target (with high attack)?
  //  //like, "target for source is now black", so with reg htp or no-0-avg wont actually head that way if other sources still active...
  //}


template<class T, class... Args>
class Driver { // shared container so I/O can yada
  private:
  std::shared_ptr<T> _driver;

  // friend RenderStage; // or not even necessary let's see...

  public:
  Driver(const String& id, Args&&... args) {
    _driver(new T(std::forward<Args>(args)...));
  }

  T& get() { return *_driver; }
};

}
