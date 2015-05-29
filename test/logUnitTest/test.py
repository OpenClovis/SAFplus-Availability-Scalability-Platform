import openclovis.test.testcase as testcase
                
class test(testcase.TestGroup):
  
    def test_2(self):
        r"""
        \testcase   LOG-BAS-PYT.TC001
        \brief     	Basic log functional tests 
        """
        self.appTest(15)  # An App Test just starts running its tests when started (there is no addtl trigger required to put the entity "in service", etc.  The parameter is how long to wait before assuming the test hung.
        self.assert_equal(1, 1, 'This test always works')
