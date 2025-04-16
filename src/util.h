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
  
class from_string {
  const std::string str;
public:
  from_string(const char* str): str(str) {}
  from_string(const std::string& str): str(str) {}
  template<typename T>
  operator T() {
      if constexpr (std::is_same_v<T, float>)
        return stof(str);
      else if      (std::is_same_v<T, int> ||
                    std::is_same_v<T, uint8_t> ||
                    std::is_same_v<T, uint16_t> ||
                    std::is_same_v<T, uint32_t> ||
                    std::is_same_v<T, int8_t> ||
                    std::is_same_v<T, int16_t> ||
                    std::is_same_v<T, int32_t>)
        return stoi(str);
      else if      (std::is_same_v<T, std::string>) // could extend concept to a parsing class
        return str;
  }
};


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
