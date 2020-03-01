# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(ENV{ESP32})
set(ENV{ARDUINO_ARCH_ESP32})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXTRA_COMPONENT_DIRS src lib lib32 externals/smooth/smooth_component)

if(${ESP_PLATFORM})
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_options (-fdiagnostics-color=always)
add_compile_options (-fconcepts)
project(pixtol)
else()
  # figure out whether can keep supporting 8266 and stuff uhh
endif()
