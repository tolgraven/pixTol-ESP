Import("env")

def reload_compile_commands(source, target, env):
    env.Execute("fish -c 'pio_compile_commands'")

env.AddPreAction("upload", reload_compile_commands)
