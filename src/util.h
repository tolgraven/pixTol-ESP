#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <sstream>

#include <esp_heap_caps.h>
#include <esp_err.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_heap_task_info.h"

#include "log.h"

namespace tol {
namespace util {
  
inline std::vector<std::string> split(std::string text, char delim) {
  std::string line;
  std::vector<std::string> vec;
  std::stringstream ss(text);
  while(std::getline(ss, line, delim)) {
      vec.push_back(line);
  }
  return vec;
} 

inline std::vector<std::string> split(std::string text, std::string delim) {
  std::vector<std::string> vec;
  size_t pos = 0, prevPos = 0;
  while(1) {
    pos = text.find(delim, prevPos);
    if(pos == std::string::npos) {
      vec.push_back(text.substr(prevPos));
      return vec;
    }

    vec.push_back(text.substr(prevPos, pos - prevPos));
    prevPos = pos + delim.length();
  }
}

// from https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/GeneralUtils.h
inline void logHeap() {
	size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  size_t minimum = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  
	lg.f("Heap", Log::DEBUG, "Free heap: %d, minimum: %d, largest block: %d\n",
      freeHeap, largestBlock, minimum);
}


static void logHeapRegions(void) {
  
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);
  
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

// 	static void        hexDump(const uint8_t* pData, uint32_t length);

inline std::string toLower(std::string& value) {
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
	return value;
} // toLower

inline std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first) return str;
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
} // trim

// XXX other interesting files
// Memory.h
// 



// or whatever name. bits n pieces.
//
//from https://github.com/nlohmann/json/issues/958
// Let's take fmt as an example, I wrote a custom formatter for json objects, but implicit conversions triggered ambiguous overloads in fmt's internals...
// The "fix" was to add the following in my own code:
// namespace nlohmann {
// template <typename C>
// fmt::internal::init<C, nlohmann::json const&, fmt::internal::custom_type>
// make_value(nlohmann::json const& j) { return {j}; }
// }

// replace arduino with this
// uint32_t millis() {
//   using namespace std::chrono;
//   uint32_t ms = duration_cast<milliseconds>(steady_clock::now() .time_since_epoch()).count();
//   return ms; 
// }

}
}



  // From Per Malmberg/smooth:
/// \brief Workaround for json::value() throwing when object is null even though
/// a default value has been provided.
/// https://github.com/nlohmann/json/issues/1733
#include <nlohmann/json.hpp>
namespace nlohmann {

#include <string>


template<typename T>
auto valueOr(json& json, const std::string& key, T default_value) {
    return json.is_object() ? json.value(key, default_value) : default_value;
}

}
