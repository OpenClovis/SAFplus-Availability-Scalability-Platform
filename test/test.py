import openclovis.test.testcase as testcase
                
class test(testcase.TestGroup):
  
    def test_1(self):
        r"""
        \testcase   TRE-EXA-TST.TC001
        \brief     	HUP Tests 
        """
#        self.start_script() 

        self.appTest(15)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
