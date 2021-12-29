#pragma once

#include <string>

#include "smooth/core/Task.h"
#include "smooth/core/ipc/IEventListener.h"
#include "smooth/core/ipc/SubscribingTaskEventQueue.h"
#include "smooth/core/ipc/Publisher.h"
#include "smooth/core/task_priorities.h"

#include "base.h"
#include "log.h"
#include <stdexcept>

namespace tol {

using namespace smooth;
using namespace smooth::core;
using namespace smooth::core::ipc;
using namespace std::chrono;

// well this is dumb apart from as experiment, but afa that got better ways to learn unpacking.
// for now too-simple sub is through creating EventFnTasks...
// capture this in lambda and got full access so w/e
//
// template<class Final, class... Evts, class Listener = IEventListener>
// template<class Final, class... Evts>
// // class Subs: public Listener<Evts>... {
// class Subs: public IEventListener<Evts>... {
//   public:
//   std::shared_ptr<SubscribingTaskEventQueue<Evts>>... queue;
//   Subs(Inheritor* self, int size = 10, bool autostart = false):
//     queue(SubscribingTaskEventQueue<Evts>::create(size, *self, *self))... {
//     }
// };

// but mostly, use ...args and unpacking and can just list shit to inherit/use
// template<class Inheritor, class T>
template<class T>
class Sub: public IEventListener<T> {
  core::Task* task;
  int size;
  public:
  std::shared_ptr<SubscribingTaskEventQueue<T>> queue;
  Sub(core::Task* task, int size = 2):
    task(task), size(size),
    queue(SubscribingTaskEventQueue<T>::create(size, *task, *this)) {}

  void disableSub() { queue.reset(nullptr); }
  void enableSub() { queue.reset(SubscribingTaskEventQueue<T>::create(size, *task, *this)); }
  virtual void event(const T& out) { onEvent(out); } // a remnant, remove...
  virtual void onEvent(const T& out) = 0;
};

class PureEvtTask: public core::Task { // Evt = Event, in what sense lol? I guess cause does nothing etc
  public:
  PureEvtTask(const char* id, int priority = 4):
    // core::Task(id, 3000, core::APPLICATION_BASE_PRIO - 10, seconds{1}, 1) { // pin cpu 1
    core::Task(id, 4096, priority, seconds{1}, 1) { // pin cpu 1
    // core::Task(id, 3000, core::APPLICATION_BASE_PRIO - 5, milliseconds{25}) {
      start();
    }
};


// if too much time passes dont forget THESE ARE DUMB however, plenty applications on plenty targets will
// make much more sense basic & cooperative, so dont give up on runnableyada. Just. Fuckem here.
class FnTask: public core::Task {
  public:
  std::function<void()> task;
  FnTask(int ms, std::function<void()> task): // anon. attach to main task. except that means we enter loop here and get fucked.
    core::Task(core::APPLICATION_BASE_PRIO - 5, milliseconds{ms}),
    task(task) { start(); }

  FnTask(const std::string& id, int ms, std::function<void()> task,
         int stackSize = 3000, int prio = core::APPLICATION_BASE_PRIO):
    core::Task(id.c_str(), stackSize, prio, milliseconds{ms}),
    task(task) { start(); }

  void tick() override { task(); }
};

template<class T> // ... args w eventlisteners?
class EventFnTask: public core::Task, public IEventListener<T> {
  public:
  std::shared_ptr<SubscribingTaskEventQueue<T>> queue;
  using EvtFn = std::function<void(const T& evt)>;
  EvtFn task;
  EventFnTask(const std::string& id, EvtFn task, int stackSize = 3000,
              int prio = core::APPLICATION_BASE_PRIO - 5):
    core::Task(id.c_str(), stackSize, prio, milliseconds{10000}),
    queue(SubscribingTaskEventQueue<T>::create(5, *this, *this)),
    task(task) {
      start();
    }
  void event(const T& evt) override { task(evt); }
};

class TaskedRunnable: public core::Task { // will auto clear on dtor right
  protected:
  std::shared_ptr<Runnable> runner; // can also just be a RunnableContainer etc for grouping.
  public:
  TaskedRunnable(Runnable* runner, int stackSize = 4096,
                 int prio = core::APPLICATION_BASE_PRIO - 5, int tick = 5, int cpu = tskNO_AFFINITY):
    core::Task((runner->id() + " " + runner->type() + std::to_string(runner->uid())).c_str(),
                stackSize, prio, milliseconds{tick}, cpu),
    runner(runner) {
      // start(); // lets chill w auto init
    }

  void init() override { }
  void tick() override { runner->run(); }

  Runnable& get() { return *(runner.get()); }
  Runnable& operator*() { return get(); }
  Runnable* operator->() { return runner.get(); }
};


class TaskRunGroup: public RunnableGroup, public core::Task { // will auto clear on dtor right
  public:
  TaskRunGroup(const std::string& id, int stackSize = 4000,
               int prio = core::APPLICATION_BASE_PRIO - 5, int tick = 5,
               int cpu = tskNO_AFFINITY):
    RunnableGroup(id),
    core::Task(id.c_str(), stackSize, prio, milliseconds{tick}, cpu) {
      // start();
    }

  void init() override { DEBUG("Start TaskRunGroup " + id()); }
  void tick() override { run(); }
};


template<class T> // like this'd solve but obv dum-dum lose are cntainer
class TaskGroup: public Runnable, public core::Task { // Q is are eg Inputters one task tot with sep threads or each single one a task?
  // guess latter since can work priority dep on timed out, high activity etc...  but in the end should be very event driven w data coming in.
  public:
  TaskGroup(const std::string& id, int priority = 5, int tick = 5, int cpu = tskNO_AFFINITY):
    Runnable(id, "tasks"),
    core::Task(id.c_str(), 4096, priority, milliseconds{tick}, cpu) {
      start(); //maybe not always safe start on reg? could also pass event to hook start to?
    }

  using Tasks = std::vector<std::shared_ptr<T>>;
  Tasks& tasks() { return _tasks; }

  void add(Runnable* task) { _tasks.emplace_back(static_cast<T*>(task)); }
  T& get(uint8_t index = 0) { return *(_tasks[index].get()); }
  T& operator[](uint8_t index) { return get(index); }
  T& get(const std::string& id) { // something something copy operator.
    for(auto&& t: _tasks)
      if(id == t->id())
        return *((T*)*t);
    throw std::invalid_argument("No such task in TaskGroup");
  }
  bool _run() override {
    bool outcome = false;
    for(auto&& task: tasks())
      outcome |= static_cast<T*>(task.get())->run(); // ok this actually breaks diamond again unless go templatizing or pass what to cast to.
    return outcome;
  }
  void tick() override { run(); }

  Tasks _tasks;
};
//  above goes away, this more sensible for groups of smallish grouped subtasks afa current dumb setup.
//  since want to keep strictly-polling base design in place for portability?
//  tho then would just do straight TaskedRunnable containing RunnableContainer...
//  so this thing can fuckoff hahah
// class TaskGroupGeneral: public RunnableContainer, public core::Task {
//   public:
//   TaskGroupGeneral(const std::string& id, int priority = 7, int cpu = tskNO_AFFINITY):
//     RunnableContainer(id),
//     core::Task(id.c_str(), 8096, priority, std::chrono::milliseconds{5}, cpu) {}

//   void tick() override { run(); } // this takes care of not-returning and that, but obviously actual tasks within need adapting through yielding and shit...
// };
class PeriodicTask: public LimitedRunnable {
  public:
  PeriodicTask(uint32_t periodMs, std::function<bool()> fn):
    LimitedRunnable("eh", "task") {
      setRunFn(fn);
      auto timer = new PeriodLimiter(periodMs);
      conditions.limiters.emplace_back(timer);
    }
};

class InterruptTask {
  Ticker timer; // TODO Ticker is a tiny wrapper lib, just replicate and excise it...
  public:
  InterruptTask(uint32_t periodMs, std::function<bool()> fn) {
    timer.attach_ms(periodMs, fn);
  }
  InterruptTask(uint32_t periodMs, uint32_t timeoutMs, std::function<void()> fn) {
    auto timeoutAt = millis() + timeoutMs;
    timer.attach_ms(periodMs, [fn = fn, timeoutAt] {
                                if(millis() < timeoutAt) {
                                  fn(); return true;
                                } else return false;});
  }
};

// TODO this should use Timer from smooth...
// schedule a fn (probably lambda wrapping something internal) in ms
// to defer firing it
class RunFnOnceIn {
  Ticker timer; // TODO Ticker is a tiny wrapper lib, just replicate and excise it...
  std::function<bool()> _fn;
  // milliseconds start = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
  milliseconds start = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
  // might also have a memleak (move callback, set it to nullptr...?) we'll find out
  // cause this will be set up and torn down a loooot?
  public:
  RunFnOnceIn(uint32_t runInMs, std::function<bool()> fn):
    _fn(fn){
    timer.once_ms(runInMs, _fn);
  }
  ~RunFnOnceIn() {
    // disarm(); // Ticker detaches on destruction already tho
  }
  
  void disarm() {
    if(timer.active()) {
      timer.detach();
    }
  }
  void setNewTimeout(uint32_t runInMs) {
    disarm();
    timer.once_ms(runInMs, _fn);
  }
  // maybe better to avoid recreating entire class and its timer object if can be avoided, so...
  void runAgainIn(uint32_t runInMs) {
    timer.once_ms(runInMs, _fn);
  }
  // change (delay or move closer) time it should fire
  // void adjustTimeout(int32_t diff) {
  //   disarm();
  //   auto passed = steady_clock::now()
  //   timer.attach_once_ms(, _fn);
  // }
};

}
