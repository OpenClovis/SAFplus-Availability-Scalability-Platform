import openclovis.test.testcase as testcase
import os
import pdb
                
class test(testcase.TestGroup):
    def dirPfx(self):
      return self.model.cfg.mapping.SysCtrl0.install_dir + os.sep + self.model.cfg.model_bin_dir    
  
    def test_group1(self):
        r"""
        \testcase   GRP-TST-PYT.TC001
        \brief     	Group functional testss
        """
        # pdb.set_trace()
        self.progTest(self.dirPfx() + "/test/testGroup",60)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
