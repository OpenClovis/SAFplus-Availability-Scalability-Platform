import os
import subprocess
import time
import sys

unload_tipc_cmd = 'modprobe -r tipc'

load_tipc_cmd = 'modprobe tipc'
# load_tipc_cmd = 'modprobe -f tipc'  # -f forces the module to load regardless of version string.  TEMPORARY

is_tipc_loaded_cmd = 'lsmod | grep tipc'

get_amf_pid_cmd = 'ps -o pid,cmd -A'

shm_dir = os.getenv('ASP_SHM_DIR') or 'dev/shm'

core_file_dir = ''
core_file_regex = 'core.*'

def system(cmd):
    """Similar to the os.system call, except that both the output and
    return value is returned"""
    if sys.version_info[0:2] <= (2, 4):
        pipe=os.popen('%s' %cmd)
        output=pipe.read()
        sts=pipe.close()
        retval=0
        signal=0
        core=0
        if sts:
            retval = int(sts)
            signal = retval & 0x7f
            core   = ((retval & 0x80) !=0)
            retval = retval >> 8
        return (retval, output, signal, core)
    else :
        child = subprocess.Popen(cmd, shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             close_fds=True)
        output = []
        while True:
            pid, sts = os.waitpid(child.pid, os.WNOHANG)
            output += child.stdout.readlines()
            if pid == child.pid:
                break
            else:
                time.sleep(0.00001)
        child.stdout.close()
        child.stderr.close()
        retval = sts
        signal = retval & 0x7f
        core   = ((retval & 0x80) !=0)
        retval = retval >> 8
        del child
        return (retval, output, signal, core)

def Popen(cmd):
    """Similar to the os.popen call, except that using subprocess.Popen from python 2.6"""
    if sys.version_info[0:2] <= (2, 4):
        return os.popen('%s' %cmd)
    else:
        child = subprocess.Popen(cmd, shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             close_fds=True)
        output = []
        while True:
            pid, sts = os.waitpid(child.pid, os.WNOHANG)
            output += child.stdout.readlines()
            if pid == child.pid:
                break
            else:
                time.sleep(0.00001)
        child.stdout.close()
        child.stderr.close()
        del child
        return output

def getMultiLink():
    """Similar to the os.system call, except that both the output and
    return value is returned"""
    output = []
    val = os.getenv('LINK_NAME')
    if val is None:
        output.append('eth0')
        num=1
        return (num,output)
    output = val.split(',')        
    num = len(output)    
    return (num,output)

def get_kill_amf_cmd(f):
    return 'killall -KILL %s 2> /dev/null' % f

def get_pid_cmd(p):
    return 'ps -e -o pid,cmd | grep \'%s\'' % p

def get_start_amf_watchdog_cmd(p):
    return 'setsid %s/safplus_watchdog.py &' % p

def get_cleanup_safplus_cmd(p):
    return 'rm -f /%s/CL*_%s' % (shm_dir, p)
