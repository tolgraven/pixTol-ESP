#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>

#include <Ticker.h>
#include <Arduino.h>

namespace tol {

// maybe reorganize as, urr, Object ;P
// add uid, ptr to parent (?) (well, creator, since can be multiple owners)
using UID = uint32_t;

class Named { // has a name and a type.
  public:
    Named(): Named("") {}
    Named(const std::string& id, const std::string& type = "");
    virtual ~Named() {}
    const std::string& id()   const { return _id; }
    const std::string& type() const { return _type; } // prob rename "kind"
    UID           uid()  const { return _uid; }
    void          setId(const std::string& newId)     { _id = newId; }
    void          setType(const std::string& newType) { _type = newType; }

    virtual std::string toString() const {
      return std::to_string(uid()) + "-> " + type() + ": " + id();
    }
  private:
    std::string _id, _type;
    UID _uid;
};


// setup with buffer(s) subdivided by elements of a specific size
// largely overlapping with Field stuff, utilize?
class ChunkedContainer {  // (Buffer, but also others containing multuple Buffers - RS)
  public:
    ChunkedContainer(uint16_t fieldCount = 0): ChunkedContainer(1, fieldCount) {}
    ChunkedContainer(uint8_t fieldSize, uint16_t fieldCount):
      _fieldSize(fieldSize), _fieldCount(fieldCount) {}
    virtual ~ChunkedContainer() { delete subFieldOrder; }
    uint8_t  fieldSize()   const { return _fieldSize;  }
    uint16_t fieldCount()  const { return _fieldCount; }
    void     setFieldSize (uint8_t  newFieldSize)  { _fieldSize  = newFieldSize;  }
    void     setFieldCount(uint16_t newFieldCount) { _fieldCount = newFieldCount; }
    uint16_t length()      const { return _fieldSize * _fieldCount; } // should be called totalSize or sizeInUnits or some shit dunno

    virtual void setSubFieldOrder(const uint8_t subFields[]);
    uint8_t subFieldForIndex(uint8_t sub) const { return subFieldOrder? subFieldOrder[sub]: sub; }
    const uint8_t* getSubFieldOrder() const { return subFieldOrder; }

    std::string toString() const {
      return "fieldSize " + std::to_string(fieldSize()) + ", fieldCount " + std::to_string(fieldCount());
    }

  protected:
    uint8_t  _fieldSize;
    uint16_t _fieldCount;
    uint8_t* subFieldOrder = nullptr;
};


// setup w chrono
struct Checkpoint {
  uint32_t time;
  uint32_t runs;

  Checkpoint(uint32_t runs = 0, uint32_t time = micros()):
    time(time), runs(runs) {}

  void update(uint32_t numRuns, uint32_t newTime = micros()) {
    time = newTime, runs = numRuns;
  }
  float hzSince(const Checkpoint& rhs = Checkpoint( 0, 0 )) {
    return (float)(time - rhs.time) / (runs - rhs.runs);
  }
}; // so uh have like five of these one updates every 5-10 frames, 1s, 5s, 1m, ya?


// thinking might step back little on this, keep lean then inherit seldom something needs to *change* M.O.
// same concepts are good forbtracking keyframes and interpolation and in some ways triggered stuff that will run anim eg strobe.
class Runnable: public Named { // something expected to be run (usually continously
  public:
    Runnable(const std::string& id, const std::string& type, uint16_t targetHz = 40): // make default targetHz build config option
      Named(id, type), targetHz(targetHz) {
      ts.start = micros(); // more like create I guess
    }
    virtual ~Runnable() {}

    bool run(); // here, maybe on throttled and def averaging basis, track actual hz
    bool operator()() { return run(); } // ???

    virtual bool ready(); // no const cause conditions actually get updated and shit
    virtual uint32_t microsTilReady() const; // so this version can only know high-level / enforced stuff
    uint32_t passed() const { return micros() - ts.run; }

    struct TimeStamps { uint32_t start, run = 0, attempt = 0; } ts;
    struct Counts { uint32_t totalTime = 0, run = 0, attempt = 0; } count;

    const TimeStamps& getTimes() const { return ts; }
    Counts& getCounts() { return count; }

    void setEnabled(bool state = true) { _enabled = state; }
    bool enabled() const { return _enabled; }
    bool active() const { return _active; } // prob better as activity()

    float averageHz(uint32_t timePeriod = 0) const { return (float)count.run / (micros() - ts.start); }

    std::string toString() const override {
      return Named::toString() + ". Runs/drops " + std::to_string(count.run) + " / " +
        std::to_string(count.attempt - count.run) + " (" + std::to_string(averageHz()) + " hz)";
    }

    void setTargetFreq(uint16_t hz) { targetHz = hz; }

    using RunFn_t = std::function<bool()>;
    void setRunFn(RunFn_t fn) { _runFn = std::move(fn); } // void setRunFn(RunFn_t fn) { _runFn = std::move(fn); }
    const RunFn_t& runFn() const { return _runFn; }
  
  protected:
    uint16_t targetHz = 0; // disabled by default. "targetHz" more sense, incl doing whatever w (actual) scheduler to ensure doens't drop below either, and offsets get compensated both dirs so no drift.

  private:
    bool _active = true, _enabled = true;
    RunFn_t _runFn;

    void checkAndHandleTimeOut();
    void setActive(bool state = true) { _active = state; } // private simple setter makes no sense hah

    virtual bool _run() { return (runFn() && runFn()()); } // nothing set should mean failure no
    // virtual bool _run() { if(runFn()) return runFn()(); else return true; }
    virtual bool _ready() { return true; }
    virtual void _onTimeout() {}; // eg artnet send would impl setting targetHz to 0.25.
    virtual void _onRestored() {};

  protected:

    uint32_t idleTimeout = 5000; // 0 for never. again fix proper timelib
    uint16_t throughput = 0; // Just use micros per byte I guess? for strip ca 10us per subpixel + 50us pause end each.
};

// just a Runnable with a stack of Runnables...
class RunnableGroup: public Runnable {
  public:
  RunnableGroup(const std::string& id):
    Runnable(id, "Runnables") {}

  using Tasks = std::vector<std::shared_ptr<Runnable>>; // well yeah i guess if want this "ze same" just put a vector in reg with one elem pointing to this. but eh
  Tasks& tasks() { return _tasks; } // beware lunatic spec Tasks ...() in Runnabel...

  void add(Runnable* task) { _tasks.emplace_back(task); }
  Runnable& get(int index) const { return *(_tasks[index].get()); }
  Runnable& operator[](int index) const { return get(index); }

  Runnable& get(const std::string& id) { // something something copy operator.
    for(auto&& t: _tasks) {
      if(id == t->id()) return *t;
    }
    throw std::invalid_argument("No such Runnable");
  }
  bool _run() final override { //this might make sense
    bool outcome = false;
    for(auto&& task: tasks()) {
      outcome |= task->run(); // ok this actually breaks diamond again unless go templatizing or pass what to cast to.
    }
    return outcome;
  }
  bool ready() final override {
    for(auto&& task: tasks()) {
      if(task->ready()) return true; // ok this actually breaks diamond again unless go templatizing or pass what to cast to.
    }
    return false;
  }

  private:
  void setRunFn(const RunFn_t& fn);  // can we hide existing shit like this?
  Tasks _tasks;
};


class LimitedRunnable: public Runnable { // took this out since is pretty useless + base ready() ought to lack side effects...
  public:
    LimitedRunnable(const std::string& id, const std::string& type):
      Runnable(id, type) {}

    bool ready() override {
      return (Runnable::ready() && conditions.ready());
    }

  protected:
    class Limiter { // surely this should itself derive from Runnable hehu
      public:
      virtual void enable(uint32_t value) = 0;
      virtual void disable() = 0;
      virtual bool ready() = 0;
    };

    class InvocationLimiter: public Limiter {
      uint16_t callsToRun = 0, callsSinceRun = 0;

      public:
      void enable(uint32_t callsBeforeRun) override {
        callsToRun = callsBeforeRun, callsSinceRun = 0;
      }
      void disable() override { enable(0); }
      bool ready() override {
        if(callsToRun <= 1) return true; // stupid, if 0 should destroy
        if(++callsSinceRun >= callsToRun) {
          callsSinceRun = 0;
          return true;
        }
        return false;
      }
    };

    // DEPRECATED
    class PeriodLimiter: public Limiter { // obvs dumb, no diff from maxHz
      Ticker timer;                    // tho multiple instances and ||'d limiters would do for patterns n shit
      bool isReady = false;

      public:
      PeriodLimiter(uint32_t ms): Limiter() {
        enable(ms);
      }
      void enable(uint32_t ms) override {
        disable();
        timer.attach_ms(ms, [this]() { this->isReady = true; });
        // ok so by using attach_scheduled can actually do "proper" stuff in cb bc not interrupt based. So can skip my entire layer here.
        // Then again point (if there even is one apart from fkn round) is setting multiple conditions for firing, I guess?
      }
      void disable() override {
        timer.detach();
        isReady = false;
      }
      bool ready() override { return (isReady? (isReady = false, true): false); }
    };

    struct {
      std::vector<std::shared_ptr<Limiter>> limiters;

      bool ready(bool any = true) {
        if(limiters.size() == 0) return true;
        bool ready = false;
        for(auto&& l: limiters) {
          // auto op = std::bind(&bool, ..., heh)
          if(any) ready |= l->ready();
          else    ready &= l->ready(); // XXX well wont work lol.
        }
        return ready;
      }
    } conditions;
};


class Timed { // eh overkill dynamic bs
  std::string _id;
  struct UpdateState {
    uint32_t updates;
    UpdateState(uint32_t currentUpdates):
      updates(currentUpdates) {}
  };

  enum Second { SMicros = 1000000, SMillis = 1000, S = 1 };
  const uint32_t checkpointInterval = SMicros; // try per s
  const uint16_t maxCheckpoints = 10;
  uint32_t start; // creation time seems important?
  uint32_t latest = 0; // or just defer to micros?
  uint32_t checkpointed = 0; // when saved
  uint32_t updates = 0; // something like polls vs actual execs. but w proper timekeeping sched can keep track i guess
  // Checkpoint start; //want it persistent
  // std::queue<Checkpoint> checkpoints; //p dumb but not idiotic. gen just check latest
  // std::array<Checkpoint, 20> checkpoints; //p dumb but not idiotic. gen just check latest
  std::vector<UpdateState> checkpoints; //p dumb but not idiotic. gen just check latest
  // should maybe auto thin and save data for long range instead :]
  // or uh ugh if known interval just array of times each...

  Timed(): start(micros()) {
    latest = start;
    checkpoints.reserve(maxCheckpoints);
  // Timed(): start(Checkpoint()) {
    // checkpoints.push(start);
  }
  ~Timed() { checkpoints.clear(); }

  void tick() {
    latest = micros();
    updates++;

    if(latest - checkpointed > checkpointInterval) {
      checkpointed = latest;
      checkpoints.emplace(checkpoints.begin(), updates);
      // checkpoints.push_(UpdateState(updates));

      if(checkpoints.size() > maxCheckpoints) {
        checkpoints.pop_back();
      }
    }
    // if(latest - checkpoints.back().timeStamp > checkpointInterval) {
    //   // checkpoints.push()
    //   checkpoints.emplace(Checkpoint(updates));
    //   if(checkpoints.size() > maxCheckpoints)
    //     checkpoints.pop();
    // }
  }
  // uint32_t getHz(uint32_t interval = 1) {
  //   // Checkpoint& cp = checkpoints.back();
  //   Checkpoint& cp = checkpoints.back();
  //   return (updates - cp.updates) / (latest - cp.timeStamp);
  // }
  uint32_t getHz(uint32_t numCheckpoints = 1) {
    // Checkpoint& cp = checkpoints.back();

    uint32_t avg = 0;
    for(auto it = checkpoints.begin();
        it != checkpoints.begin() + numCheckpoints && it != checkpoints.end(); ++it) {
      // avg += *it.updates / checkpointInterval;
      avg += it->updates / checkpointInterval;
    }
    avg /= numCheckpoints;
    return avg;
    // UpdateState& us = checkpoints.back();
    // return (updates - cp.updates) / (latest - cp.timestamp);
  }
  // uh guess, every few sec auto new checkpoint?
  // time + updatenum
  // get hz/fps avg over different timelengths, but also
  // elapsed time for num updates etc yada?
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

}
