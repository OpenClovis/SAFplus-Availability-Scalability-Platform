import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_2(self):
        r"""
        \testcase   MSG-XPT-PYT.TC002
        \brief     	Basic messaging functional tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testTransport --xport=clMsgUdp.so --mode=LAN",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_3(self):
        r"""
        \testcase   MSG-XPT-PYT.TC003
        \brief     	UDP LAN
        """
  
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgUdp.so --mode=LAN",120)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

    def test_4(self):
        r"""
        \testcase   MSG-XPT-PYT.TC004
        \brief     	UDP cloud
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testMsgServer --xport=clMsgUdp.so --mode=cloud",120)
        self.assert_equal(1, 1, 'This test always works')
