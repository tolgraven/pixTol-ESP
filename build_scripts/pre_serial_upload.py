Import("env")

def kill_monitor(source, target, env):
    # env.Execute("fish -c 'kill (lsof -t /dev/*usbserial*) >&- ^&-'")
    env.Execute("pkill -f 'pio device monitor'")
env.AddPreAction("upload", kill_monitor)
env.AddPreAction("uploadfs", kill_monitor)
