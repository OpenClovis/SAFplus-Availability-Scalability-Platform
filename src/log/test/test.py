import openclovis.test.testcase as testcase
import os
                
class test(testcase.TestGroup):
  
    def test_2(self):
        r"""
        \testcase   LOG-BAS-PYT.TC001
        \brief     	Basic log functional tests 
        """
        self.progTest(self.model.cfg.mapping.SysCtrl0.install_dir + os.sep + self.model.cfg.model_bin_dir + "/test/testLog",120)
