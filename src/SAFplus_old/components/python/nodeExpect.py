# Python imports
import os
import re

# 3rd party imports

# ASP runtime director imports
import misc
import psh
import pxsshfix as pxssh
import aspLog

log = aspLog.log

class ModuleFormatException(Exception):
  pass

class AspRunningException(Exception):
  pass



class Node(psh.Psh):
  """
  A machine.  This class represents a node (machine) in the cluster
  """

  def __init__(self, name, aspdir, ip,user,password,logdir="."):  
    self.name     = name
    self.ip       = ip
    self.user     = user
    self.password = password
    self.asp_dir  = aspdir

    if 1:
      logfilename = logdir + os.sep + "ssh_" + self.name + ".log"

      retries = 0
      while retries<4:
        retries += 1
        tgtexp = pxssh.pxssh()
        fout = open(logfilename,"w")
        tgtexp.logfile = fout
        try:
          result = tgtexp.login(self.ip, self.user, self.password, login_timeout=30)
          retries = 10000
        except pxssh.ExceptionPxssh,e: # Could not synchronize with original prompt
          if retries>3: raise misc.Error("Cannot connect to %s exception: %s" % (self.ip,str(e)))

      if not result:
          raise misc.Error("Cannot connect to %s" % self.ip)
      tgtexp.setwinsize(200, 1024)
      tgtexp.delaybeforesend = 0 # to make sessions a lot faster
      tgtexp.sendline("export PS1='_tgtexpectprompt_$ '")

      psh.Psh.__init__(self, tgtexp, re.escape('_tgtexpectprompt_$ '))
      
      self.bash = self  # for compatibility with other object hierarchies


  def asp_running(self,aspdir=None):
    """ Return true if ASP is running """
    if aspdir is None:
      aspdir = self.asp_dir
    rc, msg = self.bash.run_raw("%s/etc/init.d/asp status" % aspdir)
    log.info('ASP %s status on node [%s]: %s' % (aspdir, self.name, ' '.join(msg.splitlines())))
    return rc == 0

  def asp_status(self,aspdir=None):
    """ Return with init.d/asp status return code """
    if aspdir is None:
      aspdir = self.asp_dir
    rc, msg = self.bash.run_raw("%s/etc/init.d/asp status" % aspdir)
    return rc

  def start_asp(self, stop_first=False, start_args=None, aspdir=None):
    if aspdir is None:
      aspdir = self.asp_dir
    s = self.bash
    if stop_first:
      self.zap_asp(aspdir)

    cmd = "%s/etc/init.d/asp start"
    if start_args:
      cmd += " %s" % start_args
    s.run(cmd % aspdir, {
          "Invalid module format" : ModuleFormatException("This code was built for a different kernel then is on this machine"),
          "ASP is already running" : AspRunningException("ASP already running")},
          timeout=90)
    # Set the default ASP to be the one you started.
    self.asp_dir = aspdir

  def stop_asp(self,aspdir=None):
    if aspdir is None:
      aspdir = self.asp_dir
    s = self.bash
    s.run_raw("%s/etc/init.d/asp stop" % aspdir, timeout=60) # ec ignored!

  def unload_tipc(self):
    s = self.bash
    s.run_raw("rmmod tipc") # ec ignored!

  def zap_asp(self,aspdir=None):
    if not aspdir:
      aspdir = self.asp_dir
    s = self.bash
    s.run_raw("%s/etc/init.d/asp zap" % aspdir, timeout=60) # ec ignored!

  def restart_asp(self,aspdir=None):
    self.start_asp(stop_first=True,aspdir=aspdir)
  
