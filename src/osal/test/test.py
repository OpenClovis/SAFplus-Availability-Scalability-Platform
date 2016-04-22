import openclovis.test.testcase as testcase
import os
import pdb
                
class test(testcase.TestGroup):
    def dirPfx(self):
      return self.model.cfg.mapping.SysCtrl0.install_dir + os.sep + self.model.cfg.model_bin_dir    
  
    def test_osal(self):
        r"""
        \testcase   OSL-C-TST.TC001
        \brief     	OSAL C unit tests 
        """
        # pdb.set_trace()
        self.progTest(self.dirPfx() + "/test/testOsal",300)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.

