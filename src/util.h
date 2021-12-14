#pragma once

#include <chrono>

namespace tol::util {
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
