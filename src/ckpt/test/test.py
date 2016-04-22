import openclovis.test.testcase as testcase
import os
                
class test(testcase.TestGroup):
    def dirPfx(self):
      return self.model.cfg.mapping.SysCtrl0.install_dir + os.sep + self.model.cfg.model_bin_dir    
  
    def test_1(self):
        r"""
        \testcase   CKP-BAS-PYT.TC001
        \brief     	Basic checkpoint functional tests 
        """
        self.progTest(self.dirPfx() + "/test/testCkpt",60)
