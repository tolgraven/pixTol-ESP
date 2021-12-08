set(CMAKE_CXX_STANDARD 17)

set(COMPONENT_SRCDIRS
  .
  ./io
  # ../externals/fmt/src
  )          # remember ensure cmake rerun if adding files tho
set(COMPONENT_ADD_INCLUDEDIRS
  .
  ./io
  # ../externals/fmt/include/fmt
  )
set(COMPONENT_REQUIRES
                      lib
                      arduino
                      smooth_component
                      artnet
                      esp_gdbstub
                      )

    # /os.cc
    # /format.cc
register_component()

  # component_compile_options (-fconcepts) # wraps target_compile_options()
target_compile_options(${COMPONENT_LIB} PRIVATE
    -DESP32
    -DARDUINO_ARCH_ESP32
    -DARDUINO
    # -DSERIAL_BAUD=230400
    -DSERIAL_BAUD=${CONFIG_MONITOR_BAUD}
    -Wno-error
    # -Wno-error=unused-command-line-argument-hard-error-in-future
    # -Wno-error=unused-command-line-argument
    # cant have bc gcc errors on these lol
    )

