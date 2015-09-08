import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_amf1(self):
        r"""
        \testcase   AMF-FNC-TST.TC111
        \brief     	Availability management framework functional 1 node 1 sg 1 comp
        """
        self.progTest("python {0}/bin/dbalpy.py -x {0}/test/SAFplusAmf1Node1SG1Comp.xml safplusAmf".format(self.model.cfg.mapping.SysCtrl0.installDir),30)
        self.progTest("python " + self.model.cfg.mapping.SysCtrl0.installDir + "embTest111.py",200)
        self.assert_equal(1, 1, 'This test always works')
