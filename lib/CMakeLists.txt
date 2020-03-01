set(CMAKE_CXX_STANDARD 17)
set(COMPONENT_ADD_INCLUDEDIRS
    # AppleMIDI/src
    # AppleMIDI/src/utility
    Arduino-modded
    OpcServer/src
    Arduino-PrintStream/src
    artnet/src
    lib8tion/src
    pixTol
    FastLED
    )

file(GLOB lib AppleMIDI/src/*.cpp Arduino-modded/*.cpp Arduino-PrintStream/src/*.cpp ArtNetE131Lib_esp8266/src/*.cpp lib8tion/src/*.cpp OpcServer/src/*.cpp pixtol/*.h)
# set(COMPONENT_SRCS ${lib})
set(COMPONENT_SRCS
    # AppleMIDI/src/AppleMidi.cpp
    Arduino-modded/Ticker.cpp
    Arduino-PrintStream/src/PrintStream.cpp
    artnet/src/ArtNetE131Lib.cpp
    lib8tion/src/lib8tion.h
    OpcServer/src/OpcServer.cpp
    pixtol/field.h
    # FastLED/FastLED.cpp
    # FastLED/bitswap.cpp
    # FastLED/colorpalettes.cpp
    # FastLED/colorutils.cpp
    # FastLED/hsv2rgb.cpp
    # FastLED/lib8tion.cpp
    # FastLED/noise.cpp
    # FastLED/platforms.cpp
    # FastLED/power_mgt.cpp
    # FastLED/wiring.cpp
    )

set(COMPONENT_REQUIRES arduino)

# idf_component_register()
register_component()
# idf_component_register(SRCS "${lib}"
#     INCLUDE_DIRS AppleMIDI Arduino-modded Arduino-PrintStream ArtNetE131Lib_esp8266 lib8tion OpcServer pixTol
#                        REQUIRES arduino)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -DESP32
    -DARDUINO_ARCH_ESP32
    -DARDUINO
    -Wno-error=class-memaccess
    )
