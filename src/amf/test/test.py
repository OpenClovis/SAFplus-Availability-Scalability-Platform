import openclovis.test.testcase as testcase
import os
import pdb
                
class test(testcase.TestGroup):
    def dirPfx(self):
      return self.model.cfg.mapping.SysCtrl0.install_dir + os.sep + self.model.cfg.model_bin_dir    
  
    def test_amf1(self):
        r"""
        \testcase   AMF-FNC-UDP.TC111
        \brief     	Availability management framework functional 1 node 1 sg 1 comp, UDP
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.model.cfg.mapping.SysCtrl0.installDir),30)
        self.progTest("(pkill -9 safplus_amf; pkill -9 exampleSafApp; export SAFPLUS_MSG_TRANSPORT=clMsgUdp.so; cd " + self.dirPfx() + "/test; ../bin/safplus_cleanup; python embTest111.py)",500)

    def test_amf2(self):
        r"""
        \testcase   AMF-FNC-SCTP.TC112
        \brief     	Availability management framework functional 1 node 1 sg 1 comp, SCTP
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.dirPfx()),30)
        self.progTest("(pkill -9 safplus_amf; pkill -9 exampleSafApp; export SAFPLUS_MSG_TRANSPORT=clMsgSctp.so; cd " + self.dirPfx() + "/test; ../bin/safplus_cleanup; ../bin/safplus_cloud --add `ifconfig $SAFPLUS_BACKPLANE_INTERFACE | awk '/inet addr/{print substr($2,6)}'`; python embTest111.py)",500)

    def test_amf3(self):
        r"""
        \testcase   AMF-FNC-TIPC.TC113
        \brief     	Availability management framework functional 1 node 1 sg 1 comp, TIPC
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.dirPfx()),30)
        self.progTest("(pkill -9 safplus_amf; pkill -9 exampleSafApp; export SAFPLUS_MSG_TRANSPORT=clMsgTipc.so; modprobe tipc; tipc-config --netid=1227 --addr=1.1.1 --be=eth:$SAFPLUS_BACKPLANE_INTERFACE; cd " + self.dirPfx() + "/test; ../bin/safplus_cleanup; ../bin/safplus_cloud --add `ifconfig $SAFPLUS_BACKPLANE_INTERFACE | awk '/inet addr/{print substr($2,6)}'`; python embTest111.py)",500)

    def test_amf4(self):
        r"""
        \testcase   AMF-FNC-TCP.TC114
        \brief     	Availability management framework functional 1 node 1 sg 1 comp, TIPC
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.dirPfx()),30)
        self.progTest("(pkill -9 safplus_amf; pkill -9 exampleSafApp; export SAFPLUS_MSG_TRANSPORT=clMsgTcp.so; cd " + self.dirPfx() + "/test; ../bin/safplus_cleanup; ../bin/safplus_cloud --add `ifconfig $SAFPLUS_BACKPLANE_INTERFACE | awk '/inet addr/{print substr($2,6)}'`; python embTest111.py)",500)
