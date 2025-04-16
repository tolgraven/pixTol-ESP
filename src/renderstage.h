#pragma once
#include <vector>
#include <stdexcept>
#include <Print.h>

#include "smooth/core/ipc/TaskEventQueue.h"
#include "smooth/core/ipc/IEventListener.h"
#include "smooth/core/timer/Timer.h"
#include "smooth/core/timer/TimerExpiredEvent.h"

#include "buffer.h"
#include "base.h"
#include "task.h"

namespace tol {

class RenderStage;
  
enum class Patching { IN, OUT, INCopy, OUTCopy };
enum class PatchType { Controls, Pixels };

// needs a new name
template<Patching dir, auto dest = PatchType::Pixels>
struct BufferPatch {
  BufferPatch() = default;
  BufferPatch(const Buffer& buffer, int i): buffer(buffer), destIdx(i) {}
  // BufferPatch(const Buffer& buffer, int i): buffer(buffer, 0, 0, true), destIdx(i) {} // for now at least, yet more copying...
  // BufferPatch(Buffer& buffer, int i): buffer(&buffer), i(i) {}
  // BufferPatch& operator=(BufferPatch& rhs) {
  //   buffer = rhs.buffer; i = rhs.i; return *this;
  // }
  BufferPatch& operator=(const BufferPatch& rhs) = default;
  // Buffer* buffer = nullptr;
  Buffer buffer = Buffer(); // XXX copy needed since eg slicing in ArtnetIn, but for most stuff bit inefficient (one hand, only ptr no copy, other hand, tons of other crap in class)
  // Buffer& buffer;
  int destIdx = 0; // index for OUTGOING buffer (in future not necessarily the same as )
  // ^ should be renamed to just idx because router handles translation
  
  RenderStage* originator = nullptr;
  int priority = 10; // 0 to 20 I guess. If actively receiving messages of a higher priority, ignore lower ones?
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
    enum StageType { IN, OUT, THROUGH };
    RenderStage(const std::string& id, uint16_t numBytes): RenderStage(id, 1, numBytes) {}
    RenderStage(const std::string& id, uint8_t fieldSize, uint16_t fieldCount,
                uint8_t bufferCount = 1, uint16_t portIndex = 0, uint16_t targetHz = 40);
    virtual ~RenderStage() { _buffers.clear(); }
    virtual void init() {}

    std::string toString() const override;
    void sendDiagnostic(const std::string s) {} // impl as Stream? art has pollreply freestle text, or  opt for using whatever output...

    void setSubFieldOrder(const uint8_t subFields[]) override {
      for(auto b: buffers()) b->setSubFieldOrder(subFields);
    }

    virtual uint32_t sizeInBytes() const;

    virtual Buffer& buffer(uint8_t index = 0) const;
    Buffer& operator[](uint16_t index) { return buffer(index); } //dunno but bit convinient
    const std::vector<std::shared_ptr<Buffer>>& buffers() const { return _buffers; } // maybe add range or filter tho

    void setBuffer(Buffer& pointTo, uint8_t index = 0) {
      _buffers.at(index) = std::shared_ptr<Buffer>(&pointTo);
      // wouldn't we have to use std::move to capture any potential temporary object?
      // but then would cause other trouble hmmm
      // XXX or rather repoint the ptr? again issue w this semi-managed shit is it's all-in or nothing...
      // tho as Buffers themselves still have notion of raw data ptr ownership shouldnt result in anything hmm
      // _buffers.at(index).assign(&pointTo);
    }
    void updateBuffer(const Buffer& copyFrom, uint8_t index = 0) {
      buffer(index).setCopy(copyFrom);
    }
    
    void setGain(float gain) {
      _gain = gain;
      for(auto& b: buffers())
        b->setGain(gain);
    }
    float getGain() const { return _gain; }

    void setPortId(uint16_t port) { _portId = port; }
    uint16_t portId() { return _portId; }

    bool dirty() const;

  protected:
    std::vector<std::shared_ptr<Buffer>> _buffers; // kow I've discoved multiple multiple times why use ptrs inside.
    float  _gain = 1.0; //applies to buffers within unless overridden.
    uint16_t _portId; // to start, only continous possible...   UDP port, pin, interface id, whatever...
    uint8_t _priority = 0; //might be useful for all stages no? just had in inputter earlier
    StageType stageType;
  private:
};

class Inputter: public RenderStage {
  // std::shared_ptr<TaskEventQueue<Buffer>> sendQueue; // POSTING. pass in ctor so can post specific and not to everyone
  // std::shared_ptr<SubscribingTaskEventQueue<Buffer>> queue; // POSTING. pass in ctor so can post specific and not to everyone
  public:
    Inputter(const std::string& id, uint16_t bufferSize = 512): Inputter(id, 1, bufferSize) {} // raw bytes = field is 1
    Inputter(const std::string& id, uint8_t fieldSize, uint16_t fieldCount,
             uint8_t bufferCount = 1, uint16_t portIndex = 0,
             int controlDataLength = 0):
      RenderStage(id, fieldSize, fieldCount, bufferCount + (controlDataLength? 1: 0), portIndex) {
      setType("inputter"); // just use RTTI...
      if(controlDataLength) {
        controlsBuffer = std::make_shared<Buffer>(id + " controls", 1, controlDataLength);
      }
    }

    bool onData(uint16_t index, uint16_t length, uint8_t* data, bool flush = true);
  protected:
    std::shared_ptr<Buffer> controlsBuffer;

    virtual void onCommand() {}
};

class NetworkIn: public Inputter { // figure artnet/sacn/opc (more?) such common format could use this?
  public:
  NetworkIn(const std::string& id, uint16_t startPort, uint8_t numPorts, int controlDataLength = 0):
    Inputter(id, 1, 512, numPorts, startPort, controlDataLength) {
      setType("NetworkIn");
    } //tho some not so DMX-aligned

  protected:
  bool isFrameSynced = false; // dunno if others, but Artnet lots timecode + flush stuff.
  int lastStatusCode;
  // uint8_t lastSeq; //common enough?
};


class Outputter: public RenderStage, public core::Task, public Sub<PatchOut> {
  public:
  Outputter(const std::string& id, uint16_t bufferSize = 512):
    Outputter(id, 1, bufferSize) {} // raw bytes = field is 1
  Outputter(const std::string& id, uint8_t fieldSize, uint16_t fieldCount,
            uint8_t bufferCount = 1, uint16_t portIndex = 0, uint16_t fixedHz = 0):
    RenderStage(id, fieldSize, fieldCount, bufferCount, portIndex),
    core::Task(id + " event runner", 2560, 24,
               fixedHz? milliseconds(1000/fixedHz): seconds(10), // TODO 40 turns into 35 due to bleh, use smooth::core::timer instead...
               1),
    Sub<PatchOut>(this, bufferCount) {
      setType("outputter");
      setTargetFreq(fixedHz); // tho most of Runnable is just dumb remnant of prev DIY coop scheduling and should be cleaned up...
                              // tho it does have some timeout stuff we'll need to utilize?
      start();
    }

  enum BufferIndex: int { ALL = -1, VALID = 0 };

  virtual bool flush(int b = BufferIndex::ALL) {
    if(b >= BufferIndex::VALID)
      return flushBuffer(b);
    
    bool outcome = false;
    for(uint8_t i=0; i < buffers().size(); i++) {
      if(flushBuffer(i))
        outcome = true;
    }
    return outcome;
  }
  
  virtual void update(const Buffer& data, int i, bool doFlush = true) { 
    updateBuffer(data, i);
    if(doFlush) flush(i);
  } // so can update outside just events
  void onEvent(const PatchOut& out) {
    update(out.buffer, out.destIdx, !targetHz); // flush unless fixed hz
  }
  void tick() override { if(targetHz) flush(); } // default should be all
  private:

  virtual bool flushBuffer(uint8_t b) { return false; }; // XXX temp all should actually implement this in future (whether then flush called from event or tick)
};

// intercept PatchIn
// decide which input id goes to what (potentially multiple?) and call its update fn (and hence needs to be uniform)...
//
// or if passthrough (like to DMX), simply publish.
// but also stuff like merge universes for Renderer doing 144x4 Strip
// 
// basically would earlier get result of parsed json cfg of input - [merge/split] - output(s)
// but tbh move away from controls in same uni as strip data...

class Router: public RenderStage, public PureEvtTask, public Sub<PatchIn> {
  public:
  // Patcher
};

using namespace std::chrono;

class Generator: public Inputter, public core::Task {
  public:
    Generator(const std::string& id, uint16_t bufferSize = 1): // raw bytes = field is 1
      Generator(id, 1, bufferSize, 1) {} // raw bytes = field is 1
    Generator(const std::string& id, const RenderStage& model):
      Generator(id, model.fieldSize(), model.fieldCount(), model.buffers().size()) {}
    Generator(const std::string& id,  uint8_t fieldSize, uint16_t fieldCount, uint8_t bufferCount = 1, uint16_t targetHz = 40):
      Inputter(id, fieldSize, fieldCount, bufferCount),
      core::Task(id + " generator", 3072, 10, milliseconds(1000/targetHz), 1) {
        setType("Generator");
      }

    // void setDuration(std::chrono::duration<> dur) {}
    void setDuration(milliseconds dur) { playDuration = dur; }
    void restart();


    using GeneratorUpdater = std::function<bool(std::vector<std::shared_ptr<Buffer>>, float)>;
    GeneratorUpdater updater;
    void tick() override {
      duration now = milliseconds(millis());
      float progress = playDuration.count() / (float)(now.count() - ts.start);
      progress = std::clamp(progress, 0.0f, 1.0f);
      bool res = true;
      if(updater) { // lambda ting
        res = updater(buffers(), progress);
      } else { // inheritor ting
        // _run(progress);
      }


      if(now + milliseconds(1000/targetHz) > playDuration) { // about to run over
        switch(onEnd) {
          case Stop: res = false; break;
          case Loop: break;
          case BackAndForth: break;
          case Random: break;
        }
      }

      if(!res) {
        // send signal to manager we are to be killed
      }
      // update anim or whatever...
    }

    milliseconds playDuration = seconds(1);

    enum GeneratorType { Static, Playback }; // well actually anything static should not be a task n shit yeah...
    GeneratorType genType = Playback;
    enum OnEnd { Stop, Loop, BackAndForth, Random }; //if Animation or, specifically, Playback. Or tbh up to whoever is requesting generator's services?
    OnEnd onEnd = Stop;

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
  Driver(const std::string& id, Args&&... args) {
    _driver(new T(std::forward<Args>(args)...));
  }

  T& get() { return *_driver; }
};

}
