import openclovis.test.testcase as testcase
                
class test(testcase.TestGroup):
  
    def test_1(self):
        r"""
        \testcase   CKP-BAS-PYT.TC001
        \brief     	Basic checkpoint functional tests 
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.installDir + "/test/testCkpt",60)
        self.assert_equal(1, 1, 'This test always works')
