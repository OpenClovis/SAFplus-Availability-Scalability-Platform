# This wrapper around the C code lets us change the organization of the namespace in Python
import pySAFplus as sp
import amfctrl


from pySAFplus import SafplusInitializationConfiguration, Libraries, Initialize, logMsgWrite, LogSeverity, HandleType, Handle, WellKnownHandle, Transaction, SYS_LOG, APP_LOG, Buffer, Checkpoint, mgtGet, mgtSet, mgtCreate, mgtDelete, logSeverity

def zz__bootstrap__():
   global __bootstrap__, __loader__, __file__
   import sys, pkg_resources, imp
   __file__ = pkg_resources.resource_filename(__name__,'pySAFplus.so')
   __loader__ = None; del __bootstrap__, __loader__
   imp.load_dynamic("pySAFplus",__file__)
# __bootstrap__()
