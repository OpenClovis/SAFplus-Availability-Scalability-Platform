import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_amf1(self):
        r"""
        \testcase   AMF-FNC-TST.TC111
        \brief     	Availability management framework functional 1 node 1 sg 1 comp, UDP
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.model.cfg.mapping.SysCtrl0.installDir),30)
        self.progTest("(pkill -9 safplus_amf; export SAFPLUS_MSG_TRANSPORT=clMsgUdp.so; cd " + self.model.cfg.mapping.SysCtrl0.installDir + "/test; ../bin/safplus_cleanup; python embTest111.py)",300)
        self.assert_equal(1, 1, 'This test always works')

    def test_amf2(self):
        r"""
        \testcase   AMF-FNC-TST.TC112
        \brief     	Availability management framework functional 1 node 1 sg 1 comp, SCTP
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.model.cfg.mapping.SysCtrl0.installDir),30)
        self.progTest("(pkill -9 safplus_amf; export SAFPLUS_MSG_TRANSPORT=clMsgSctp.so; cd " + self.model.cfg.mapping.SysCtrl0.installDir + "/test; ../bin/safplus_cleanup; ../bin/safplus_cloud --add `ifconfig $SAFPLUS_BACKPLANE_INTERFACE | awk '/inet addr/{print substr($2,6)}'`; python embTest111.py)",300)
        self.assert_equal(1, 1, 'This test always works')

    def test_amf3(self):
        r"""
        \testcase   AMF-FNC-TST.TC113
        \brief     	Availability management framework functional 1 node 1 sg 1 comp, TCP
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.model.cfg.mapping.SysCtrl0.installDir),30)
        self.progTest("(pkill -9 safplus_amf; export SAFPLUS_MSG_TRANSPORT=clMsgTcp.so; cd " + self.model.cfg.mapping.SysCtrl0.installDir + "/test; ../bin/safplus_cleanup; python embTest111.py)",300)
        self.assert_equal(1, 1, 'This test always works')
