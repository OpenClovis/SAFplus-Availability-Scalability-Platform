import openclovis.test.testcase as testcase
                
class test(testcase.TestGroup):
  
    def test_2(self):
        r"""
        \testcase   LOG-BAS-PYT.TC001
        \brief     	Basic log functional tests 
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testLog",20)
        self.assert_equal(1, 1, 'This test always works')
