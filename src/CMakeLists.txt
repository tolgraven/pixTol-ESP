set(CMAKE_CXX_STANDARD 17)

set(COMPONENT_SRCS
    # apps/taxilight.cpp
    base.cpp
    buffer.cpp
    color.cpp
    debuglog.cpp
    event.cpp
    functions.cpp
    io/strip.cpp
    iot.cpp
    log.cpp
    pixtol.cpp
    renderer.cpp
    renderstage.cpp
    scheduler.cpp
    watchdog.cpp
    )

# add_compile_options (-fconcepts)
idf_component_register(SRCS ${COMPONENT_SRCS}
                       INCLUDE_DIRS "."
                       REQUIRES
                        mbedtls
                        lib
                        lib32
                        arduino
                        smooth_component
                       )

# target_compile_options(smooth_component PRIVATE
#     -DESP32
#     -DARDUINO_ARCH_ESP32
#     -DARDUINO
#     -Wno-error
#     )
target_compile_options(${COMPONENT_LIB} PRIVATE
    -DESP32
    -DARDUINO_ARCH_ESP32
    -DARDUINO
    -DSERIAL_BAUD=115200
    )

