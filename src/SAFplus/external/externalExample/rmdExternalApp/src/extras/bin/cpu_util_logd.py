#!/usr/bin/python

import commands
import time
import sys
import os

gNumIter = 300
fileName = '/tmp/perf_log_data.txt'
asp_dir = '/root/asp'
sysCtrlName = 'SysControllerI0'
sysCtrlResFile = '/tmp/log_sys_cpu_data.txt'
workerResFile = '/tmp/log_wkr_cpu_data.txt'

if len(sys.argv) > 1 :
    if sys.argv[1] == sysCtrlName:
        fileName = sysCtrlResFile
    else:
        fileName = workerResFile

if len(sys.argv) > 2 :
    gNumIter = int(sys.argv[2]) * 10


def get_process_pid(p):
    
    rc, psRes = commands.getstatusoutput('ps -A | grep %s' %p)

    if psRes == "":
        return 0

    pid = psRes.split()[0]
    return pid 

#----------------------------------------------------------------------
def calAvgCpuUtil():

    tmpFile = open(fileName, "w")

    pidOfLogd = get_process_pid('safplus_logd')

    if pidOfLogd == 0 :
        tmpFile.write('ERROR : Unable to find pid of safplus_logd\n')
        return

    totalCpuUtil=0.0
    maxCpuUtil=0.0
    minCpuUtil=99.9

    time.sleep(3)
    tmpFile.write('Calculating CPU Util. of safplus_logd for ' + str(gNumIter/10) + ' Secs.\n')
    tmpFile.write('Please wait...\n')
    tmpFile.flush();

    for i in range(1, gNumIter + 1):
        rc, cpuUtil = commands.getstatusoutput('ps -o pcpu -p %s' % pidOfLogd) 
        if rc <> 0 :
            tmpFile.write('ERROR : Unable to get CPU util. of safplus_logd\n')
            return
        cpuUtil = float(cpuUtil.split()[1].strip())
        totalCpuUtil += cpuUtil
        if maxCpuUtil < cpuUtil :
            maxCpuUtil = cpuUtil
        if minCpuUtil > cpuUtil :
            minCpuUtil = cpuUtil
        time.sleep(0.1)
    
    avgCpuUtil = totalCpuUtil/gNumIter

    res = "\nCPU Utilization of Log Service in percentage :- \n\nAvg : [" + str(round(avgCpuUtil, 3)) + "%]\nMax : [" + str(maxCpuUtil) + "%]\nMin : [" + str(minCpuUtil) + "%]\n"
    tmpFile.write(res)            
    tmpFile.write("\nTest Done.\n")
    tmpFile.flush();   
    return
#----------------------------------------------------------------------
calAvgCpuUtil()
#----------------------------------------------------------------------
