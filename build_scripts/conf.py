Import("env")
from subprocess import call
from base64 import b64decode
from SCons.Script import ARGUMENTS, AlwaysBuild


def reset_and_post(*args, **kwargs):
    env.Execute("rsync $DATA_DIR/config.json pi@paj:pixtol/configs/") # cp to paj for deployment, and snapshots (so move last out of way first)
    # send mqtt cmd to reset unit
    # tell paj to temp connect (only) wlan0 to resulting network, and POST json
    # so from pi:
    curl = 'curl -X PUT http://192.168.123.1/config --header "Content-Type: application/json" -d @config.json'
    env.Execute("ssh pi@paj " + curl)
    # reconnect wlan0 to internet...
