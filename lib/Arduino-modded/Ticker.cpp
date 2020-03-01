/*
  Ticker.cpp - esp8266 library that calls functions periodically

  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef ESP8266
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#endif

#include "Ticker.h"

typedef void (*callback_with_arg_t)(void*);
typedef std::function<void(void)> callback_function_t;
#ifdef ESP8266
void __disarm(TickerInternal timer) {
    if(timer) {
        os_timer_disarm(timer);
    }
}
void __attach(TickerInternal timer, uint32_t milliseconds, bool repeat, callback_with_arg_t callback, void* arg) {
    os_timer_setfn(timer, callback, arg);
    os_timer_arm(timer, milliseconds, repeat);
}
#endif

#ifdef ESP32
void __disarm(TickerInternal timer) {
    if(timer) {
        esp_timer_stop(timer);
        esp_timer_delete(timer);
    }
}

void __attach(TickerInternal timer, uint32_t milliseconds, bool repeat, callback_with_arg_t callback, void* arg) {
  esp_timer_create_args_t _timerConfig;
  // _timerConfig.arg = reinterpret_cast<void*>(arg);
  _timerConfig.arg = arg;
  _timerConfig.callback = callback;
  _timerConfig.dispatch_method = ESP_TIMER_TASK;
  _timerConfig.name = "Ticker";
  __disarm(timer);
  esp_timer_create(&_timerConfig, &timer);
  if (repeat) {
    esp_timer_start_periodic(timer, milliseconds * 1000ULL);
  } else {
    esp_timer_start_once(timer, milliseconds * 1000ULL);
  }
}
#endif

Ticker::Ticker()
    : _timer(nullptr) {}

Ticker::~Ticker()
{
    detach();
}

void Ticker::_attach_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, void* arg)
{
    __disarm(_timer);
#ifdef ESP8266
    _timer = &_etsTimer;
#endif
    __attach(_timer, milliseconds, repeat, callback, arg);
}

void Ticker::detach()
{
    __disarm(_timer);
    _timer = nullptr;
    _callback_function = nullptr; // is this not memleak recipe after a ::move??
}

bool Ticker::active() const
{
    return _timer;
}

void Ticker::_static_callback(void* arg)
{
    Ticker* _this = reinterpret_cast<Ticker*>(arg);
    if (_this && _this->_callback_function)
        _this->_callback_function();
}
