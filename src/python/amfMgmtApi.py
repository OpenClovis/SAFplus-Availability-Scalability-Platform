import time
import signal
import cmd
import pdb
import re
#import commands
import subprocess
import socket
import fcntl
import struct
import safplus as s
import microdom
import amfctrl
import os
import localaccess as access
from lxml import etree as et


#dummyHandle = access.getProcessHandle(0, 0)
def getSafplusInstallInfo(nodeName):
    #global access
    amfMgmtHandle = access.Handle.create(os.getpid())
    ret = access.amfMgmtInitialize(amfMgmtHandle)
    if ret==0x0:
        installInfo = access.amfMgmtSafplusInstallInfoGet(amfMgmtHandle, nodeName)
        access.amfMgmtFinalize(amfMgmtHandle, True)
        #access.Finalize()
        if not '0x' in installInfo and len(installInfo)>0:
            d = dict([x.split('=') for x in installInfo.split(',')])        
            #print (installInfo)
            #d = dict(installInfo)
            #print ('ipaddr:', d.get('interface').split(':')[1])
            return (d.get('interface').split(':')[1], d.get('dir'))
        else:
            print ('getting safplus install info error: %s'%installInfo)
    else:
        print ('amfMgmt init error:0x%x'%ret)
    return ("","")

def setSafplusInstallInfo(nodeName, *args):
    if len(args) < 4:
        return 0x02 # CL_ERR_INVALID_PARAMETER
    
    localIp = args[0]
    aspDir = args[1]
    interface = args[2]
    version = args[3]

    amfMgmtHandle = access.Handle.create(os.getpid())
    ret = access.amfMgmtInitialize(amfMgmtHandle)
    if ret==0x0:
        safplusInstallInfo = "interface=%s:%s,version=%s,dir=%s" % (interface, localIp, version, aspDir)

        rc = access.amfMgmtSafplusInstallInfoSet(amfMgmtHandle, nodeName, safplusInstallInfo)
        access.amfMgmtFinalize(amfMgmtHandle, True)
        if rc == 0x0:
            print("setting safplus install info succedded")
        else:
            print("amfMgmtSafplusInstallInfoSet returned error code: " + hex(rc))

    else:
        print ('amfMgmt init error:0x%x'%ret)
    return ret