import openclovis.test.testcase as testcase
import pdb, os
                
class test(testcase.TestGroup):
    def dirPfx(self):
      return self.model.cfg.mapping.SysCtrl0.install_dir + os.sep + self.model.cfg.model_bin_dir    

    def test_rpc1(self):
        r"""
        \testcase   RPC-PRF-IPC.TC001
        \brief     	Interprocess Remote Procedure Call performance test
        """
        # pdb.set_trace()
        self.progTest(self.dirPfx() + "/test/testRpcPerf",300)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
