#pragma once

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_heap_task_info.h"

#include "smooth/core/logging/log.h"
#include "smooth/core/Application.h"
#include "smooth/core/Task.h"
#include "smooth/core/ipc/Publisher.h"

#include "log.h"
#include "task.h"

using namespace smooth;
using namespace smooth::core;
using namespace std::chrono;

namespace tol {
// class so other tasks can publish logmessages and not interrupt what they're doing,
// instead it will be output when everything has run and there is free cycle time
// (obviously need something to ensure still gets out at all if starving...)
// but since we'll potentially log over mqtt, serial, to web...
// and want to avoid formatting a complete string including location/tag and level too early...
//
// also holds some more intensive system logging formatting functions so they don't affect
// or overflow other threads
template<class Str = std::string>
struct LogMessage {
  LogMessage() = default;
  LogMessage(const Str& tag, const Str& level, const Str& msg):
    tag(tag), level(level), msg(msg) {} // should ::move here no?
  LogMessage& operator=(const LogMessage& rhs) = default;
  Str tag, level, msg;
};

class Logger: public core::Task,
              public Sub<LogMessage<>>,
              public Sub<LogMessage<const char*>>,
              public Sub<LogMessage<String>> {
  public:
  Logger():
    core::Task("Logger", 6144, 1, seconds(60), 0),
    Sub<LogMessage<>>(this, 10),
    Sub<LogMessage<const char*>>(this, 10),
    Sub<LogMessage<String>>(this, 10) {
      start();
    }

  void tick() override {
    top();
    logHeap();
    // logHeapRegions();
  }

  void onEvent(const LogMessage<const char*>& msg) override {
    lg.log(msg.msg, Log::Level::INFO, msg.tag);
  }
  void onEvent(const LogMessage<std::string>& msg) override {
    lg.log(msg.msg.c_str(), Log::Level::INFO, msg.tag.c_str()); // XXX for now. fix
  }
  void onEvent(const LogMessage<String>& msg) override {
    lg.log(msg.msg, Log::Level::INFO, msg.tag); // for now. fix
  }

  // follow up with a forwarder fmt wrapper ya
  template<class Str = std::string>
  static void log(const Str& tag, const Str& level, const Str& msg) {
    ipc::Publisher<LogMessage<Str>>::publish(LogMessage<Str>(tag, level, msg));
  } // all logging should be done like this because ensures at least publishing doesnt hog cycles
    // even better with a forwarder because then most of assembly (in most instances) also low prio

  // top-esque view of tasks.
  static void top() {
    auto numTasks = uxTaskGetNumberOfTasks(); /* Take a snapshot of the number of tasks in case it changes while this function is executing. */
    TaskStatus_t taskStats[numTasks]; // should be np auto work over malloc/free? w enough stack
    std::string s;
    std::map<uint32_t, std::string> m;
    auto heap = getHeapRegions();

    uint32_t timeTot;
    uxTaskGetSystemState(taskStats, numTasks, &timeTot);
    timeTot /= 100UL; /* For percentage calculations. */
    constexpr const char* fmtStr = "{:<25} {:<14} {}{}%\t {} \t {}\t{}\t{}\t{}\t{}\r\n";
    if(timeTot > 0) { /* Avoid divide by zero errors. */
      for(auto& task: taskStats) { /* Create a human readable table from the binary data. */
        uint32_t percent = task.ulRunTimeCounter / timeTot;
        auto state = "";
        switch(task.eCurrentState) {
          case eRunning:   state = "RUN"; break;
          case eReady:     state = "READY"; break;
          case eBlocked:   state = "BLOCK"; break;
          case eSuspended: state = "SUS"; break;
          case eDeleted:   state = "DEL"; break;
          default: state = "INVLD";
        }
        
        auto time = fmt::format("{:%H:%M:%S}", microseconds(task.ulRunTimeCounter));

        auto s = fmt::format(fmtStr,
              task.pcTaskName, time, percent > 0UL? " ": "~", percent,
              state, task.uxCurrentPriority, task.xTaskNumber,
              (task.xCoreID == 0 || task.xCoreID == 1)? std::to_string(task.xCoreID): " ",
              task.usStackHighWaterMark, heap[task.xTaskNumber]);
        m[task.ulRunTimeCounter] = s; //prev was sexier but oorder for 0
        // vTaskDelay(5); // or rather, spin off to a low prio logger task.
      }
    }
    for(auto& t: m) s = t.second + s;
    auto res = fmt::format(fmtStr, "TASK", "TIME", " ", " ", "STATE",
                              "PRIO", "#", "CPU", "MIN STACK", "HEAP") + s;
    ipc::Publisher<LogMessage<decltype(res)>>::publish(LogMessage<>("top", "INFO", "\n" + res));
  }

  //std::array<size_t, 3> getHeap() {
  //  //
  //}

  static void logHeap() {
    size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    size_t minimum = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    
    auto res = fmt::format("Free heap: {}, minimum: {}, largest block: {}\n",
        freeHeap, largestBlock, minimum);
    ipc::Publisher<LogMessage<>>::publish(LogMessage<>("Heap", "INFO", res));
  }
  

  static heap_task_info_params_t& getHeapRegionsSystem() { // move elsewhere...
      const uint8_t maxTaskNum = 20;
      const uint8_t maxBlockNum = 20;

      static heap_task_totals_t s_totals_arr[maxTaskNum];
      static heap_task_block_t s_block_arr[maxBlockNum];
      
      static size_t s_prepopulated_num = 0; // shouldnt this mean if start new tasks they no sho?
      
      static heap_task_info_params_t heap_info = {};
      heap_info.caps[0] = MALLOC_CAP_8BIT;        // Gets heap with CAP_8BIT capabilities
      heap_info.mask[0] = MALLOC_CAP_8BIT;
      heap_info.caps[1] = MALLOC_CAP_32BIT;       // Gets heap info with CAP_32BIT capabilities
      heap_info.mask[1] = MALLOC_CAP_32BIT;
      heap_info.tasks = NULL;                     // Passing NULL captures heap info for all tasks
      heap_info.num_tasks = 0;
      heap_info.totals = s_totals_arr;            // Gets task wise allocation details
      heap_info.num_totals = &s_prepopulated_num;
      heap_info.max_totals = maxTaskNum;          // Maximum length of "s_totals_arr"
      heap_info.blocks = s_block_arr;             // Gets block wise allocation details. For each block, gets owner task, address and size
      heap_info.max_blocks = maxBlockNum;         // Maximum length of "s_block_arr"

      heap_caps_get_per_task_info(&heap_info);

      return heap_info;
  }

  static std::map<uint32_t, size_t> getHeapRegions() {
    auto& regions = getHeapRegionsSystem();
    std::map<uint32_t, size_t> numToHeap;

    for(int i = 0 ; i < *regions.num_totals; i++) {
      lg.log(("num " + std::to_string(uxTaskGetTaskNumber(regions.totals[i].task)) + "heap "
            + std::to_string(regions.totals[0].size[0]) + "\n").c_str(),
          Log::Level::INFO, "FART"); // for now. fix
      numToHeap.insert({uxTaskGetTaskNumber(regions.totals[i].task),
                        regions.totals[i].size[0]});
    }
    return numToHeap;
  }
  
  static void logHeapRegions() {
    auto& regions = getHeapRegionsSystem();

    std::string res = "Current allocations\n";
    
    lg.log(std::to_string(regions.totals[0].size[0]).c_str(), Log::Level::INFO, "FART"); // for now. fix

    // suddenly num_totals is 20 instead of actual amt of tasks?
    for(int i = 0 ; i < *regions.num_totals; i++) {
      res += fmt::format("CAP_8BIT: {}\tCAP_32BIT: {}\t-> Task: {}\n",
                regions.totals[i].size[0],   // Heap size with CAP_8BIT capabilities
                regions.totals[i].size[1],   // Heap size with CAP32_BIT capabilities
                regions.totals[i].task? pcTaskGetTaskName(regions.totals[i].task)?
                                        pcTaskGetTaskName(regions.totals[i].task): "NULL":
                                          "Pre-Scheduler allocs");
    }

    ipc::Publisher<LogMessage<>>::publish(LogMessage<>("Heap", "INFO", res + "\n"));
  }
  
};

}
