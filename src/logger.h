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
    tag(tag), level(level), msg(msg) {}
  LogMessage& operator=(const LogMessage& rhs) = default;
  Str tag, level, msg;
};

class Logger: public core::Task,
              public Sub<LogMessage<>>,
              public Sub<LogMessage<String>> {
  public:
  Logger():
    core::Task("Logger", 8192, 1, seconds(60), 1),
    Sub<LogMessage<>>(this, 20),
    Sub<LogMessage<String>>(this, 20) {
      start();
    }

  void tick() override {
    top();
  }

  void onEvent(const LogMessage<std::string>& msg) override {
    lg.log(msg.msg.c_str(), Log::Level::INFO, msg.tag.c_str()); // for now. fix
  }
  void onEvent(const LogMessage<String>& msg) override {
    lg.log(msg.msg, Log::Level::INFO, msg.tag); // for now. fix
  }

  // top-esque view of tasks.
  static void top() {
    auto numTasks = uxTaskGetNumberOfTasks(); /* Take a snapshot of the number of tasks in case it changes while this function is executing. */
    TaskStatus_t taskStats[numTasks]; // should be np auto work over malloc/free? w enough stack
    std::string s;
    std::map<uint32_t, std::string> m;

    uint32_t timeTot;
    uxTaskGetSystemState(taskStats, numTasks, &timeTot);
    timeTot /= 100UL; /* For percentage calculations. */
    constexpr const char* fmtStr = "{:<25} {:<14} {}{}%\t {} \t {}\t{}\t{}\t{}\r\n";
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
              task.usStackHighWaterMark);
        m[task.ulRunTimeCounter] = s; //prev was sexier but oorder for 0
        vTaskDelay(5); // or rather, spin off to a low prio logger task.
      }
    }
    for(auto& t: m) s = t.second + s;
    auto res = fmt::format(fmtStr, "TASK", "TIME", " ", " ", "STATE",
                              "PRIO", "#", "CPU", "MIN STACK") + s;
    // ipc::Publisher<decltype(res)>::publish(LogMessage<decltype(res)>())
    ipc::Publisher<LogMessage<decltype(res)>>::publish(LogMessage<std::string>("top", "INFO", res));
  }

  static void logHeap() {
    size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    size_t minimum = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    
    lg.f("Heap", Log::DEBUG, "Free heap: %d, minimum: %d, largest block: %d\n",
        freeHeap, largestBlock, minimum);
  }


  static void logHeapRegions(void) {
    
      const uint8_t maxTaskNum = 20;
      const uint8_t maxBlockNum = 20;

      static heap_task_totals_t s_totals_arr[maxTaskNum];
      static heap_task_block_t s_block_arr[maxBlockNum];
      
      static size_t s_prepopulated_num = 0;
      
      
      heap_task_info_params_t heap_info = {{0}};
      heap_info.caps[0] = MALLOC_CAP_8BIT;        // Gets heap with CAP_8BIT capabilities
      heap_info.mask[0] = MALLOC_CAP_8BIT;
      heap_info.caps[1] = MALLOC_CAP_32BIT;       // Gets heap info with CAP_32BIT capabilities
      heap_info.mask[1] = MALLOC_CAP_32BIT;
      heap_info.tasks = NULL;                     // Passing NULL captures heap info for all tasks
      heap_info.num_tasks = 0;
      heap_info.totals = s_totals_arr;            // Gets task wise allocation details
      heap_info.num_totals = &s_prepopulated_num;
      heap_info.max_totals = maxTaskNum;        // Maximum length of "s_totals_arr"
      heap_info.blocks = s_block_arr;             // Gets block wise allocation details. For each block, gets owner task, address and size
      heap_info.max_blocks = maxBlockNum;       // Maximum length of "s_block_arr"

      heap_caps_get_per_task_info(&heap_info);

      for(int i = 0 ; i < *heap_info.num_totals; i++) {
          lg.f("Heap", Log::DEBUG, "CAP_8BIT: %d\tCAP_32BIT: %d\t-> Task: %s\n",
                  heap_info.totals[i].size[0],    // Heap size with CAP_8BIT capabilities
                  heap_info.totals[i].size[1],   // Heap size with CAP32_BIT capabilities
                  heap_info.totals[i].task ? pcTaskGetTaskName(heap_info.totals[i].task) : "Pre-Scheduler allocs");
      }
  }
  
};

}
