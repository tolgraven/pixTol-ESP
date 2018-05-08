Import("env")
from subprocess import call
from base64 import b64decode
from SCons.Script import ARGUMENTS, AlwaysBuild

# Upload actions
#
# def before_upload(source, target, env):
#     print "before_upload"
# env.AddPreAction("upload", before_upload)

# Custom actions when building program/firmware
#
# env.AddPreAction("buildprog", callback...)
# env.AddPostAction("buildprog", callback...)
# env.AddPostAction("size", env.VerboseAction(" ".join(["curl", 
#    "-F", "upload=@$BUILD_DIR/firmware.bin", 
#    "-F", "description='pixtol'", 
#    "http://$OTA_HOST:9080/upload"
#    ]), "Uploading $BUILD_DIR/firmware.bin using homie"))

# Custom actions for specific files/objects
#
def homie_ota_upload(source, target, env):
    host = b64decode(ARGUMENTS.get("OTA_HOST"))
    env.Execute("curl -F upload=@$BUILD_DIR/firmware.bin -F description='pixtol' http://" + host + ":9080/upload")
    env.Execute("pkill pio")

env.AddPostAction("$BUILD_DIR/firmware.bin", homie_ota_upload)

# custom action before building SPIFFS image. For example, compress HTML, etc.
def spiffs_ota_homie(source, target, env):
    host = b64decode(ARGUMENTS.get("OTA_HOST"))
    env.Execute("cp $BUILD_DIR/spiffs.bin $PROJECT_DIR/firmware/")
    print "\nCopied spiffs.bin to fw dir.\n"
    # env.Execute("curl -F upload=@$PROJECT_DIR/firmware/spiffs.bin -F description='pixtol' http://" + host + ":9080/upload")
    # env.Execute("esptool.py lala")
env.AddPostAction("$BUILD_DIR/spiffs.bin", spiffs_ota_homie)

# custom action for project's main.cpp
# env.AddPostAction("$BUILD_DIR/src/main.cpp.o", callback...)

# Custom HEX from ELF
# env.AddPostAction( "$BUILD_DIR/firmware.elf",
#     env.VerboseAction(" ".join([
#         "$OBJCOPY", "-O", "ihex", "-R", ".eeprom",
#         "$BUILD_DIR/firmware.elf", "$BUILD_DIR/firmware.hex"
#     ]), "Building $BUILD_DIR/firmware.hex")
# )

def runota_callback(*args, **kwargs):
    print "Hello PlatformIO!"
    host = b64decode(ARGUMENTS.get("OTA_HOST"))
    # mode = b64decode(ARGUMENTS.get("OTA_MODE")) #arduino, homie-ota, homie2?
    # print "OTA host: ", host
    env.Execute("cp $BUILD_DIR/firmware.bin $PROJECT_DIR/firmware/")
    env.Execute("curl -F upload=@$PROJECT_DIR/firmware/firmware.bin -F description='pixtol' http://" + host + ":9080/upload")
AlwaysBuild(env.Alias("ota", "", runota_callback))

def print_env(*args, **kwargs):
    print env.Dump()
AlwaysBuild(env.Alias("env", "", print_env))
def print_targets(*args, **kwargs):
    print "Current build targets", map(str, BUILD_TARGETS)
AlwaysBuild(env.Alias("targets", "", print_targets))
