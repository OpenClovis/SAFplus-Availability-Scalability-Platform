import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_amf1(self):
        r"""
        \testcase   AMF-FNC-TST.TC111
        \brief     	Availability management framework functional 1 node 1 sg 1 comp
        """
        # self.progTest("{0}/bin/safplus_db -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.model.cfg.mapping.SysCtrl0.installDir),30)
        self.progTest("(cd " + self.model.cfg.mapping.SysCtrl0.installDir + "/test; python embTest111.py)",300)
        self.assert_equal(1, 1, 'This test always works')
