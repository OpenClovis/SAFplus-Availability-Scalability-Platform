import pdb
import time, socket
import datetime
import openclovis.test.testcase as testcase
import openclovis.test.node     as node
import xml.parsers.expat
import re

KillAsp = ["/root/asp/etc/init.d/asp restart", 10]

ConfigedInterfaces = ["eth0:0","eth0:10","eth0:11","eth0:12","eth0:13","eth0:14","eth0:15","eth0:16","eth0:17","eth0:18","eth0:19","eth0:40"]
NodeNames          = ["ctrlI0","ctrlI1","workI0","workI1","workI2","workI3","workI4","workI5","workI6","workI7","workI8","workI9"]
ConfigedIps        = ['192.168.2.10', '192.168.2.11', '192.168.2.12', '192.168.2.13', '192.168.2.14', '192.168.2.15', 
                      '192.168.3.10', '192.168.3.11','192.168.3.12','192.168.3.13','192.168.3.14','192.168.3.15','192.168.3.16','192.168.3.17','192.168.3.18','192.168.3.19',
                      '192.168.4.10', '192.168.4.11', '192.168.4.12', '192.168.4.13', '192.168.4.14', '192.168.4.15', '192.168.4.16', '192.168.4.17', '192.168.4.18']


def ListEqual(a,b):
  if len(a) != len(b): return False
  for (ai,bi) in zip(a,b):
    if ai != bi: return False
  return True

def Flatten(lst,none=True):
  ret = []
  for l in lst:
    if type(l) is type(ret):
      ret += Flatten(l,none)
    elif none or l is not None:
      ret.append(l)
    else:
      pass
  return ret

class Failed(Exception):
    pass


class VlcFailovers(testcase.TestCase): # The name of the class does not matter

    def infoLog(self,s):
        self.log.info( str(datetime.datetime.now()) + ':  ' + s)

    # This function is executed before each test_xxxx member function is called
    def set_up(self):

        self.totalKills = 0
        self.testKills  = 0
        self.iters = 5

        # self.fixture is a large data structure that models the whole
        # chassis or cluster (i.e. all of the machines ASP runs on).
        fix = self.fixture
  
        self.shell =  [ fix.nodes[x].get_bash() for x in NodeNames]
        #self.shell =  (fix.nodes["ctrl0"].get_bash(),fix.nodes["ctrl1"].get_bash())
    
    def ok(self):        
        ifaces = [ [x.ifconfig(y)['ip'] for y in ConfigedInterfaces] for x in self.shell]
        #print ifaces
        ifaces = Flatten(ifaces,none=False)
        ifaces.sort()
        print ifaces
        if ListEqual(ifaces,ConfigedIps): return True
        return False

    def wait_til_ok(self,waitTime=30):
        i = 0
        while i <= waitTime:
          if self.ok():
              return
          time.sleep(2)
          i+=1
        raise Failed()

    def killer(self, killcmd, blade):
        if type(killcmd) is type(""):
            killcmd = [killcmd]
            
        self.infoLog('Failover #%d (of this test %d).  Killing %s' % (self.totalKills,self.testKills,str(blade)))
        self.totalKills += 1
        self.testKills  += 1
        for cmd in killcmd:
            if type(cmd) is type(0):
                time.sleep(cmd)
            else:
                blade.run(cmd)

    def test_kill_alt(self): # methods with 'test' prefix are testcases
        r"""
        \testcase   STR-RES-ALT.TC001
        \brief      Test restart ASP
        \description
        Test that the IP addresses move correctly
        """
        self.testKills  = 0
        for i in range(0,self.iters):
          for s in self.shell:
            self.wait_til_ok()
            self.killer(KillAsp,s)


