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

from lxml import etree as et

access = None
try:
    import localaccess
    access = localaccess    
except ImportError as e:
    print (e)
    pass

access.Initialize()


#dummyHandle = access.getProcessHandle(0, 0)
def getSafplusInstallInfo(nodeName):
    global access
    amfMgmtHandle = access.Handle.create(os.getpid())
    ret = access.amfMgmtInitialize(amfMgmtHandle)
    if ret==0x0:
        installInfo = access.amfMgmtSafplusInstallInfoGet(amfMgmtHandle, nodeName)
        access.amfMgmtFinalize(amfMgmtHandle, True)
        access.Finalize()
        if not '0x' in installInfo:
            d = dict([x.split('=') for x in installInfo.split(',')])        
            #print (installInfo)
            #d = dict(installInfo)
            #print ('ipaddr:', d.get('interface').split(':')[1])
            return (d.get('interface').split(':')[1], d.get('dir'))
        else:
            print ('getting safplus install info error: %s'%installInfo)
    else:
        print ('amfMgmt init error:0x%x'%ret)
    return ()
