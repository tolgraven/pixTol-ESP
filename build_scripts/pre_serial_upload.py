Import("env")
from subprocess import call

# def before_upload(source, target, env):
#     print "before_upload"
# env.AddPreAction("upload", before_upload)

# Custom actions when building program/firmware
#
# env.AddPreAction("buildprog", callback...)
# env.AddPostAction("buildprog", callback...)

# env.AddPreAction("upload", env.VerboseAction(" ".join(["kill",
#     "(lsof -t /dev/tty.wchusbserial14530)",
#     ]), "killing serial monitor..."))


# Custom actions for specific files/objects


def kill_monitor(source, target, env):
    call("fish -c 'kill (lsof -t /dev/tty.*usbserial*)'")

# env.AddPreAction("upload", kill_monitor)
# env.AddPostAction("$BUILD_DIR/firmware.bin", homie_ota_upload)

# env.AddPostAction("$BUILD_DIR/firmware.hex", callback...)


# custom action before building SPIFFS image. For example, compress HTML, etc.
# env.AddPreAction("$BUILD_DIR/spiffs.bin", callback...)

# custom action for project's main.cpp
# env.AddPostAction("$BUILD_DIR/src/main.cpp.o", callback...)

# Custom HEX from ELF
# env.AddPostAction( "$BUILD_DIR/firmware.elf",
#     env.VerboseAction(" ".join([
#         "$OBJCOPY", "-O", "ihex", "-R", ".eeprom",
#         "$BUILD_DIR/firmware.elf", "$BUILD_DIR/firmware.hex"
#     ]), "Building $BUILD_DIR/firmware.hex")
# )
