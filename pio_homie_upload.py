Import("env")
from subprocess import call

print "Current build targets", map(str, BUILD_TARGETS)

# Upload actions
#
# def before_upload(source, target, env):
#     print "before_upload"
#
# env.AddPreAction("upload", before_upload)


#
# Custom actions when building program/firmware
#
# env.AddPreAction("buildprog", callback...)
# env.AddPostAction("buildprog", callback...)

env.AddPostAction("size", env.VerboseAction(" ".join(["curl", 
    "-F", "upload=@$BUILD_DIR/firmware.bin", 
    "-F", "description='d1-artnet-homie'", 
    "http://paj:9080/upload"
    ]), "Uploading $BUILD_DIR/firmware.bin using homie"))


#
# Custom actions for specific files/objects
#
def homie_ota_upload(source, target, env):
    # print env
    call("curl -F upload=@$BUILD_DIR/firmware.bin -F description='d1 artnet homie' http://paj:9080/upload")

# env.AddPostAction("upload", homie_ota_upload)
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
