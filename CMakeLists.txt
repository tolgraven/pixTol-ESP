# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.17)

# set(ENV{ESP32})
# set(ENV{ARDUINO_ARCH_ESP32})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXTRA_COMPONENT_DIRS
  src
  lib
  lib/artnet
  externals/smooth/smooth_component)

##
#  BTW if <whatever thing> parses this directly wouldnt the wrapper syntax
#  fuck shit up? But then I dont think anything does, meh
#  investigate "include yada across components as before idf 4.0"
##
if(${ESP_PLATFORM})
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_options (-Wno-error)
add_compile_options (-fdiagnostics-color=always)
# add_compile_options (-fconcepts)
project(pixtol)

# # FunctionalPlus submodule test
# # include(ExternalProject)
# externalproject_add(functional_plus
#   GIT_REPOSITORY https://github.com/Dobiasd/FunctionalPlus.git
#   GIT_TAG master
#   SOURCE_DIR "${PROJECT_PATH}/externals/fplus"
#   CONFIGURE_COMMAND ""
#   BUILD_COMMAND ""
#   INSTALL_COMMAND ""
#   LOG_DOWNLOAD ON
#   LOG_BUILD ON
# )
# set(FPLUS_INCLUDE_DIR ${PROJECT_PATH}/externals/fplus/include)
# include_directories(${FPLUS_INCLUDE_DIR})
# # add_executable(main src/main.cpp)
# add_dependencies(main functional_plus)

else()
  # figure out whether can keep supporting 8266 and stuff uhh
endif()
