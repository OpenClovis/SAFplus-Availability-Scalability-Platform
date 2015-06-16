import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_udp1(self):
        r"""
        \testcase   MSG-UDP-LAN.TC001
        \brief     	UDP LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --xport=clMsgUdp.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_udp2(self):
        r"""
        \testcase   MSG-UDP-CLD.TC002
        \brief     	UDP cloud Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --xport=clMsgUdp.so --mode=cloud",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_udp3(self):
        r"""
        \testcase   MSG-UDP-LAN.TC003
        \brief     	UDP LAN Message software stack test
        """
  
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgUdp.so --mode=LAN",120)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_udp4(self):
        r"""
        \testcase   MSG-UDP-CLD.TC004
        \brief     	UDP cloud message software stack test
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgUdp.so --mode=cloud",120)
        self.assert_equal(1, 1, 'This test always works')



    def test_sctp1(self):
        r"""
        \testcase   MSG-SCT-LAN.TC001
        \brief     	SCTP LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --xport=clMsgSctp.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_sctp2(self):
        r"""
        \testcase   MSG-SCT-CLD.TC002
        \brief     	SCTP CLOUD Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --xport=clMsgSctp.so --mode=cloud",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_sctp3(self):
        r"""
        \testcase   MSG-SCT-LAN.TC003
        \brief     	SCTP LAN software stack
        """
  
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgSctp.so --mode=LAN",120)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_sctp4(self):
        r"""
        \testcase   MSG-SCT-CLD.TC004
        \brief     	SCTP cloud software stack
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgSctp.so --mode=cloud",120)
        self.assert_equal(1, 1, 'This test always works')



    def xxxtest_TCP1(self):  # TCP LAN makes no sense! (no broadcast so you need the node list)
        r"""
        \testcase   MSG-TCP-LAN.TC001
        \brief     	TCP LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --loglevel=error --xport=clMsgTcp.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_TCP2(self):
        r"""
        \testcase   MSG-TCP-CLD.TC002
        \brief     	TCP CLOUD Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --loglevel=error --xport=clMsgTcp.so --mode=cloud",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def xxxtest_TCP3(self): # TCP LAN makes no sense (no broadcast so you need the node list)
        r"""
        \testcase   MSG-TCP-LAN.TC003
        \brief     	TCP LAN software stack
        """
  
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --loglevel=error --xport=clMsgTcp.so --mode=LAN",120)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_TCP4(self):
        r"""
        \testcase   MSG-TCP-CLD.TC004
        \brief     	TCP cloud software stack
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --loglevel=error --xport=clMsgTcp.so --mode=cloud",120)
        self.assert_equal(1, 1, 'This test always works')



    def test_TPC1(self):
        r"""
        \testcase   MSG-TPC-LAN.TC001
        \brief     	TPC LAN Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --xport=clMsgTipc.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_TPC2(self):
        r"""
        \testcase   MSG-TPC-CLD.TC002
        \brief     	TPC CLOUD Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --xport=clMsgTipc.so --mode=cloud",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_TPC3(self):
        r"""
        \testcase   MSG-TPC-LAN.TC003
        \brief     	TPC LAN software stack
        """
  
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgTipc.so --mode=LAN",120)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_TPC4(self):
        r"""
        \testcase   MSG-TPC-CLD.TC004
        \brief     	TPC cloud software stack
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgTipc.so --mode=cloud",120)
        self.assert_equal(1, 1, 'This test always works')


