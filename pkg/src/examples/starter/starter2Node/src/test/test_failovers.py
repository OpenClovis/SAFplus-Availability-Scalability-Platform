import pdb
import time, socket
import datetime
import openclovis.test.testcase as testcase
import openclovis.test.node     as node
import xml.parsers.expat
import re

KillAsp = ["/root/asp/etc/init.d/asp restart", 10]

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
  
        self.shell =  (fix.nodes["Node0"].get_bash(),fix.nodes["Node1"].get_bash())
    
        #self.shell[0].ifconfig = ifconfig
        #self.shell[1].ifconfig = ifconfig
 

    def ok(self):
        ifaces = [x.ifconfig("eth0:0") for x in self.shell]
        #print ifaces
        #pdb.set_trace()
        ifaces = [x["ip"] for x in ifaces]
        #print ifaces
        # Should be exactly one mapped ethernet
        if "192.168.1.200" in ifaces and None in ifaces:
            return 1
        return 0

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
            
        self.infoLog('Failover #%d (of this test %d).  Killing %d' % (self.totalKills,self.testKills,blade))
        self.totalKills += 1
        self.testKills  += 1
        for cmd in killcmd:
            if type(cmd) is type(0):
                time.sleep(cmd)
            else:
                self.shell[blade].run(cmd)

    def test_kill_havlc_alt(self): # methods with 'test' prefix are testcases
        r"""
        \testcase   VLC-KIL-ALT.TC001
        \brief      Test killing the havlc process
        \description
        Always kills primary
        """
        self.testKills  = 0
        for i in range(0,self.iters):
          self.wait_til_ok()
          self.killer(KillAsp,0)
          self.wait_til_ok()
          self.killer(KillAsp,1)

