Import("env")
from subprocess import call
from base64 import b64decode
from SCons.Script import ARGUMENTS, AlwaysBuild

def fw_copy(source, target, env):
    env.Execute("cp $BUILD_DIR/firmware.bin $PROJECT_DIR/firmware/stash/")
    print "\nCopied firmware.bin to homie-ota subdir.\n"
env.AddPostAction("$BUILD_DIR/firmware.bin", fw_copy)
