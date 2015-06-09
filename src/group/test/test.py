import openclovis.test.testcase as testcase
import pdb
                
class test(testcase.TestGroup):
  
    def test_group1(self):
        r"""
        \testcase   GRP-TST-PYT.TC001
        \brief     	Group functional testss
        """
        # pdb.set_trace()
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testGroup",60)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')
