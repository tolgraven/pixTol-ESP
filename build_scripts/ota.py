from base64 import b64decode
from SCons.Script import ARGUMENTS, AlwaysBuild
Import("env")
# Upload actions
# def before_upload(source, target, env):
#     print "before_upload"
# env.AddPreAction("upload", before_upload)

# Custom actions when building program/firmware
# env.AddPreAction("buildprog", callback...)
# env.AddPostAction("buildprog", callback...)


def homie_ota_upload(source, target, env):
    host = b64decode(ARGUMENTS.get("OTA_HOST"))
    # unit = b64decode(ARGUMENTS.get("UNIT"))
    print "\nUploading firmware.bin to ota server.\n"
    env.Execute("curl -F upload=@$BUILD_DIR/firmware.bin -F description='pixtol' http://" + host + ":9080/upload")

    # figure out what's wrong...
    # print "\nFlashing firmware.bin over mqtt...\n"
    # env.Execute("homie_ota_updater -t pixtol/ -i " + unit + " $BUILD_DIR/firmware.bin")
    # env.Executej("pkill pio")

env.AddPostAction("$BUILD_DIR/firmware.bin", homie_ota_upload)


# custom action before building SPIFFS image. For example, compress HTML, etc.
def spiffs_ota_homie(source, target, env):
    host = b64decode(ARGUMENTS.get("OTA_HOST"))
    env.Execute("cp $BUILD_DIR/spiffs.bin $PROJECT_DIR/firmware/stash/")
    print "\nCopied spiffs.bin to fw dir.\n"
    # env.Execute("curl -F upload=@$PROJECT_DIR/firmware/spiffs.bin -F description='pixtol' http://" + host + ":9080/upload")
    # env.Execute("esptool.py lala")
env.AddPostAction("$BUILD_DIR/spiffs.bin", spiffs_ota_homie)


# def runota_callback(*args, **kwargs):
def runota_callback(source, target, env):
    host = b64decode(ARGUMENTS.get("OTA_HOST"))
    unit = b64decode(ARGUMENTS.get("UNIT"))
    # mode = b64decode(ARGUMENTS.get("OTA_MODE")) #arduino, homie-ota, homie2?
    env.Execute("cp $BUILD_DIR/firmware.bin $PROJECT_DIR/firmware/stash/")
    # env.Execute("curl -F upload=@$PROJECT_DIR/firmware/stash/firmware.bin -F description='pixtol' http://" + host + ":9080/upload")
    # env.Execute("pkill esptool")
    env.Execute("homie_ota_updater -t pixtol/ -i " + unit + "$BUILD_DIR/firmware.bin")
AlwaysBuild(env.Alias("ota", "", runota_callback))


def print_env(*args, **kwargs):
    print env.Dump()
AlwaysBuild(env.Alias("env", "", print_env))


def print_targets(*args, **kwargs):
    print "Current build targets", map(str, BUILD_TARGETS)
AlwaysBuild(env.Alias("targets", "", print_targets))


# env.AddPostAction("$BUILD_DIR/src/main.cpp.o", callback...)   # custom action for project's main.cpp

# env.AddPostAction( "$BUILD_DIR/firmware.elf",     #Custom HEX from ELF
#     env.VerboseAction(" ".join([ "$OBJCOPY", "-O", "ihex", "-R", ".eeprom",
#         "$BUILD_DIR/firmware.elf", "$BUILD_DIR/firmware.hex" ]), "Building $BUILD_DIR/firmware.hex"))
