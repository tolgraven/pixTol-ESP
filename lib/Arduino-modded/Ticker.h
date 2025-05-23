/*
  Ticker.h - esp8266 library that calls functions periodically

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

#ifndef TICKER_H
#define TICKER_H

#include <functional>

#if defined(ESP8266)
#include <Schedule.h>
#include <ets_sys.h>
using TickerInternal = ETSTimer*;

#elif defined(ESP32)
extern "C" {
  #include "esp_timer.h"
}
using TickerInternal = esp_timer_handle_t;
#endif


class Ticker
{
public:
    Ticker();
    ~Ticker();

    typedef void (*callback_with_arg_t)(void*);
    typedef std::function<void(void)> callback_function_t;

    // void attach_scheduled(float seconds, callback_function_t callback)
    // {
    //     _callback_function = [callback]() { schedule_function(callback); };
    //     _attach_ms(1000UL * seconds, true);
    // }

    void attach(float seconds, callback_function_t callback)
    {
        _callback_function = std::move(callback);
        _attach_ms(1000UL * seconds, true);
    }

    // void attach_ms_scheduled(uint32_t milliseconds, callback_function_t callback)
    // {
    //     _callback_function = [callback]() { schedule_function(callback); };
    //     _attach_ms(milliseconds, true);
    // }

    void attach_ms(uint32_t milliseconds, callback_function_t callback)
    {
        _callback_function = std::move(callback);
        _attach_ms(milliseconds, true);
    }

    template<typename TArg>
    void attach(float seconds, void (*callback)(TArg), TArg arg)
    {
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wcast-function-type"
        static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
        _attach_ms(1000UL * seconds, true, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
// #pragma GCC diagnostic pop
    }

    template<typename TArg>
    void attach_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg)
    {
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wcast-function-type"
        static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
        _attach_ms(milliseconds, true, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
// #pragma GCC diagnostic pop
    }

    // void once_scheduled(float seconds, callback_function_t callback)
    // {
    //     _callback_function = [callback]() { schedule_function(callback); };
    //     _attach_ms(1000UL * seconds, false);
    // }

    void once(float seconds, callback_function_t callback)
    {
        _callback_function = std::move(callback);
        _attach_ms(1000UL * seconds, false);
    }

    // void once_ms_scheduled(uint32_t milliseconds, callback_function_t callback)
    // {
    //     _callback_function = [callback]() { schedule_function(callback); };
    //     _attach_ms(milliseconds, false);
    // }

    void once_ms(uint32_t milliseconds, callback_function_t callback)
    {
        _callback_function = std::move(callback);
        _attach_ms(milliseconds, false);
    }

    template<typename TArg>
    void once(float seconds, void (*callback)(TArg), TArg arg)
    {
        static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
        _attach_ms(1000UL * seconds, false, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
    }

    template<typename TArg>
    void once_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg)
    {
        static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
        _attach_ms(milliseconds, false, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
    }

    void detach();
    bool active() const;

protected:
    static void _static_callback(void* arg);
    void _attach_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, void* arg);
    void _attach_ms(uint32_t milliseconds, bool repeat)
    {
        _attach_ms(milliseconds, repeat, _static_callback, this);
    }
    callback_function_t _callback_function = nullptr;
    TickerInternal _timer;
#ifdef ESP8266
private:
    ETSTimer _etsTimer;
#endif
};


#endif //TICKER_H

