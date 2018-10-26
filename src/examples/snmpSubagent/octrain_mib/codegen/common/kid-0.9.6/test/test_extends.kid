<?xml version='1.0' encoding='utf-8'?>
<?python

import test.test_def as test_def

?>
<testdoc xmlns:py="http://purl.org/kid/ns#"
         py:extends="test_def, 'test_match.kid'">
  <test>
    <attempt py:content="inline_test()"/>
    <expect><p>a test paragraph</p></expect>
  </test>

  <test>
    <attempt>${inline_test()} and then ${'some'}</attempt>
    <expect><p>a test paragraph</p> and then some</expect>
  </test>
  
  <test>
    <attempt><foobar>This content should be copied</foobar></attempt>
    <expect><div class="foobar">second parent - first parent - This content should be copied</div></expect>
  </test>

  <!-- XXX need a recursive test -->
</testdoc>
