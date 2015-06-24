import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_osal(self):
        r"""
        \testcase   OSL-C-TST.TC001
        \brief     	OSAL C unit tests 
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testOsal",160)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')

