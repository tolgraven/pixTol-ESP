#pragma once

#include "smooth/core/Task.h"
#include "smooth/core/task_priorities.h"
#include "smooth/core/SystemStatistics.h"
#include "smooth/core/Application.h"
#include "smooth/core/task_priorities.h"
#include <iostream>
#include <string>

using namespace smooth::core;
class App: public Runnable, public smooth::core::Application {
  public:
    App(): Runnable("pixtol", "app main"),
           Application(APPLICATION_BASE_PRIO, std::chrono::milliseconds(20)) {}
    void init() override {
      logging::Log::info("App::Init", "Starting...");
      setup();
    }
    void tick() override { loop(); }
};

