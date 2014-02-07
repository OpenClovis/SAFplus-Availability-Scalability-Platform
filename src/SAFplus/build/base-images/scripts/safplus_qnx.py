import os

unload_tipc_cmd = ''

load_tipc_cmd = ''

is_tipc_loaded_cmd = 'echo tipc'

get_amf_pid_cmd = 'pidin'

shm_dir = '/dev/shmem'

core_file_dir = ''
core_file_regex = '*.core'

def system(cmd):
    """Similar to the os.system call, except that both the output and
    return value is returned"""

    retval = signal = core = 0
    output = os.popen(cmd).readlines()
    return (retval, output, signal, core)

def Popen(cmd):
    """Similar to the os.system call, except that both the output and
    return value is returned"""
    retval, output, signal, core = system(cmd)
    return output

def get_kill_amf_cmd(f):
    return 'slay -9 %s 2> /dev/null' % os.path.basename(f)

def get_pid_cmd(p):
    return 'pidin -F "%%a %%A" | grep \'%s\'' % p

def get_start_amf_watchdog_cmd(p):
    return 'on -d %s/safplus_watchdog.py' % p

def get_cleanup_safplus_cmd(p):
    return 'rm -f /dev/shmem/CL*_%s' % p
