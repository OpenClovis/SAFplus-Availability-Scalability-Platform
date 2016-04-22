import openclovis.test.testcase as testcase
import pdb
import os
                
class test(testcase.TestGroup):
  
    def dirPfx(self):
      return self.model.cfg.mapping.SysCtrl0.install_dir + os.sep + self.model.cfg.model_bin_dir    

    def test_udp1(self):
        r"""
        \testcase   MSG-UDP-LAN.TC001
        \brief     	UDP LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.dirPfx() + "/test/testTransport --xport=clMsgUdp.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_udp2(self):
        r"""
        \testcase   MSG-UDP-CLD.TC002
        \brief     	UDP cloud Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.dirPfx() + "/test/testTransport --xport=clMsgUdp.so --mode=cloud",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_udp3(self):
        r"""
        \testcase   MSG-UDP-LAN.TC003
        \brief     	UDP LAN Message software stack test
        """
  
        self.progTest(self.dirPfx() + "/test/testMsgServer --sar=0 --xport=clMsgUdp.so --mode=LAN --duration=7000",300)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_udp4(self):
        r"""
        \testcase   MSG-UDP-CLD.TC004
        \brief     	UDP cloud message software stack test
        """
        self.progTest(self.dirPfx() + "/test/testMsgServer --sar=0 --xport=clMsgUdp.so --mode=cloud --duration=7000",300)

    def test_sctp1(self):
        r"""
        \testcase   MSG-SCT-LAN.TC001
        \brief     	SCTP LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        # Some embedded nodes are incapable of large messages -- they run out of memory and the kernel kill -9's the process (check /var/log/messages to be sure)
        args = ""
        if self.fixture.nodes["SysCtrl0"].has_key("maxMsgSize"): 
          args += " --maxsize=%s" % str(self.fixture.nodes["SysCtrl0"].maxMsgSize)
        self.progTest(self.dirPfx() + "/test/testTransport %s --xport=clMsgSctp.so --mode=LAN" % args,160,"pkill -9 testTransport")  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_sctp2(self):
        r"""
        \testcase   MSG-SCT-CLD.TC002
        \brief     	SCTP CLOUD Basic messaging functional tests 
        """
        # pdb.set_trace()
        # Some embedded nodes are incapable of large messages -- they run out of memory and the kernel kill -9's the process (check /var/log/messages to be sure)
        args = ""
        if self.fixture.nodes["SysCtrl0"].has_key("maxMsgSize"): 
          args += " --maxsize=%s" % str(self.fixture.nodes["SysCtrl0"].maxMsgSize)

        self.progTest(self.dirPfx() + "/test/testTransport %s --xport=clMsgSctp.so --mode=cloud" % args,160,"pkill -9 testTransport")  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_sctp3(self):
        r"""
        \testcase   MSG-SCT-LAN.TC003
        \brief     	SCTP LAN software stack
        """
  
        self.progTest(self.dirPfx() + "/test/testMsgServer --sar=0 --xport=clMsgSctp.so --mode=LAN --duration=7000",300,"pkill -9 testMsgServer")  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_sctp4(self):
        r"""
        \testcase   MSG-SCT-CLD.TC004
        \brief     	SCTP cloud software stack
        """
        self.progTest(self.dirPfx() + "/test/testMsgServer --sar=0 --xport=clMsgSctp.so --mode=cloud --duration=7000",300,"pkill -9 testMsgServer")

    def test_sctp7(self):
        r"""
        \testcase   MSG-SCT-SAR.TC005
        \brief     	SCTP LAN software stack
        """
  
        self.progTest(self.dirPfx() + "/test/testMsgServer --sar=1 --xport=clMsgSctp.so --mode=LAN --duration=20000",300,"pkill -9 testMsgServer")  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_sctp8(self):
        r"""
        \testcase   MSG-SCT-SAR.TC006
        \brief     	SCTP cloud software stack
        """
        self.progTest(self.dirPfx() + "/test/testMsgServer --sar=1 --xport=clMsgSctp.so --mode=cloud --duration=20000",300,"pkill -9 testMsgServer")

    def xxxtest_TCP1(self):  # TCP LAN makes no sense! (no broadcast so you need the node list)
        r"""
        \testcase   MSG-TCP-LAN.TC001
        \brief     	TCP LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.dirPfx() + "/test/testTransport --loglevel=error --xport=clMsgTcp.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_TCP2(self):
        r"""
        \testcase   MSG-TCP-CLD.TC002
        \brief     	TCP CLOUD Basic messaging functional tests 
        """
        # Some embedded nodes are incapable of large messages -- they run out of memory and the kernel kill -9's the process (check /var/log/messages to be sure)
        args = ""
        if self.fixture.nodes["SysCtrl0"].has_key("maxMsgSize"): 
          args += " --maxsize=%s" % str(self.fixture.nodes["SysCtrl0"].maxMsgSize)
        self.progTest(self.dirPfx() + "/test/testTransport %s --loglevel=error --xport=clMsgTcp.so --mode=cloud" % args,300)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def xxxtest_TCP3(self): # TCP LAN makes no sense (no broadcast so you need the node list)
        r"""
        \testcase   MSG-TCP-LAN.TC003
        \brief     	TCP LAN software stack
        """
  
        self.progTest(self.dirPfx() + "/test/testMsgServer --loglevel=error --xport=clMsgTcp.so --mode=LAN --duration=7000",300)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_TCP4(self):
        r"""
        \testcase   MSG-TCP-CLD.TC004
        \brief     	TCP cloud software stack
        """
        self.progTest(self.dirPfx() + "/test/testMsgServer --loglevel=error --xport=clMsgTcp.so --mode=cloud --duration=7000",300)

    def test_TPC1(self):
        r"""
        \testcase   MSG-TPC-LAN.TC001
        \brief     	TPC LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        # os.system('modprobe -a tipc')
        self.progTest(self.dirPfx() + "/test/testTransport --xport=clMsgTipc.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_TPC2(self):
        r"""
        \testcase   MSG-TPC-CLD.TC002
        \brief     	TPC CLOUD Basic messaging functional tests 
        """
        # pdb.set_trace()
        # os.system('modprobe -a tipc')
        self.progTest(self.dirPfx() + "/test/testTransport --xport=clMsgTipc.so --mode=cloud",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_TPC3(self):
        r"""
        \testcase   MSG-TPC-LAN.TC003
        \brief     	TPC LAN software stack
        """
        # os.system('modprobe -a tipc')
        self.progTest(self.dirPfx() + "/test/testMsgServer --xport=clMsgTipc.so --mode=LAN --duration=7000",300)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

    def test_TPC4(self):
        r"""
        \testcase   MSG-TPC-CLD.TC004
        \brief     	TPC cloud software stack
        """
        # os.system('modprobe -a tipc')
        self.progTest(self.dirPfx() + "/test/testMsgServer --xport=clMsgTipc.so --mode=cloud --duration=7000",300)


