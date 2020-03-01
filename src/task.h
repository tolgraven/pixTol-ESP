#pragma once

#include "smooth/core/Task.h"
#include "smooth/core/ipc/IEventListener.h"
#include "smooth/core/task_priorities.h"

#include "base.h"

using namespace smooth;
using namespace std::chrono;
using ms = std::chrono::milliseconds;

const int AppBasePrio = core::APPLICATION_BASE_PRIO;


class OSTask: public Runnable, public core::Task {
  public:
  OSTask(const String& id, int stackSize = 8000, int prio = AppBasePrio):
    Runnable(id, "task"),
    core::Task(id.c_str(), stackSize, prio, milliseconds{10}) {}
  void tick() override { run(); }
};

class TaskedRunnable: public core::Task { // will auto clear on dtor right
  protected:
  std::shared_ptr<Runnable> runner; // can also just be a RunnableContainer etc for grouping.
  public:
  // TaskedRunnable(Runnable* runner, int stackSize = 8000, std::chrono::milliseconds tick = 5, int priority = core::APPLICATION_BASE_PRIO):
  // TaskedRunnable(Runnable* runner, int stackSize = 8000, auto tick = milliseconds(5), int prio = AppBasePrio + 3):
  TaskedRunnable(Runnable* runner, int stackSize = 8000, milliseconds tick = milliseconds{5}, int prio = AppBasePrio):
    core::Task((runner->id() + " task").c_str(), stackSize, prio, tick),
    runner(runner) {
      start();
    }
  void tick() override { runner->run(); }
};

// class EventRunnable:
//   public TaskedRunnable,
//   public smooth::core::ipc::IEventListener<Buffer> { // test...

//   public:
//   EventRunnable(Runnable* runner, int stackSize = 8000, int priority = core::APPLICATION_BASE_PRIO):
//     TaskedRunnable(runner, stackSize, milliseconds(1000), priority) { // well rather basically infinity, should only dispatch by event right
//     }

//   void event(const Buffer& updated) {
//     // so eh first do test impl for Buffer copier... when creating Patch would use Buffer uid to lookup which dest(s) to route to...
//   }
// };

template<class T> // like this'd solve but obv dum-dum lose are cntainer
class TaskGroup: public Runnable, public core::Task { // Q is are eg Inputters one task tot with sep threads or each single one a task?
  // guess latter since can work priority dep on timed out, high activity etc...  but in the end should be very event driven w data coming in.
  public:
  TaskGroup(const String& id, int priority = 7, int cpu = tskNO_AFFINITY):
    Runnable(id, "tasks"),
    core::Task(id.c_str(), 8096, priority, milliseconds{5}, cpu) {
      start(); //maybe not always safe start on reg? could also pass event to hook start to?
    }

  using Tasks = std::vector<std::shared_ptr<T>>;
  Tasks& tasks() { return _tasks; }

  void add(Runnable* task) { _tasks.emplace_back(static_cast<T*>(task)); }
  T& get(uint8_t index = 0) { return *(_tasks[index].get()); }
  T& operator[](uint8_t index) { return get(index); }
  T& get(const String& id) { // something something copy operator.
    for(auto&& t: _tasks)
      if(id == t->id())
        return *((T*)*t);
    throw std::invalid_argument();
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
//   TaskGroupGeneral(const String& id, int priority = 7, int cpu = tskNO_AFFINITY):
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
  Ticker timer;
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

