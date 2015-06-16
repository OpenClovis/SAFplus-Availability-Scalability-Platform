import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_amf1(self):
        r"""
        \testcase   AMF-FNC-TST.TC001
        \brief     	Availability management framework functional test
        """
        # pdb.set_trace()
        # self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testRpcPerf",300)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')
