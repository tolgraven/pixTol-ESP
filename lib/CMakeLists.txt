set(CMAKE_CXX_STANDARD 17)
set(COMPONENT_ADD_INCLUDEDIRS
    Arduino-modded
    ESPAsyncE131_ID1781
    NeoPixelBus_ID547/src/internal # get git version ffs. apply rmt patch...
    NeoPixelBus_ID547/src
    NeoPixelBus/src/internal # get git version ffs. apply rmt patch...
    NeoPixelBus/src
    OpcServer/src
    pixTol
    WifiManager
    )

# needs to run idf.py reconfigure if adding files when doing this.
# set(COMPONENT_SRCDIRS
#   )
set(COMPONENT_SRCS # cant use both apparently...
    Arduino-modded/Ticker.cpp
    ESPAsyncE131_ID1781/RingBuf.c
    ESPAsyncE131_ID1781/ESPAsyncE131.cpp
    NeoPixelBus_ID547/src/internal/RgbwColor.cpp # test. dont think need any at all actually
    NeoPixelBus_ID547/src/internal/Esp32_i2s.c # test. dont think need any at all actually
    NeoPixelBus/src/internal/RgbwColor.cpp # test. dont think need any at all actually
    NeoPixelBus/src/internal/Esp32_i2s.c # test. dont think need any at all actually
    OpcServer/src/OpcServer.cpp
    WifiManager/WiFiManager.cpp
    )

set(COMPONENT_REQUIRES arduino)

register_component()
target_compile_options(${COMPONENT_LIB} PRIVATE
    -DESP32
    -DARDUINO_ARCH_ESP32
    -DARDUINO
    -Wno-error=class-memaccess
    -Wno-extra
    -Wno-error
    )

set_source_files_properties(NeoEspBitBangMethod.h
    PROPERTIES COMPILE_FLAGS
    -Wno-ignored-qualifiers
)
