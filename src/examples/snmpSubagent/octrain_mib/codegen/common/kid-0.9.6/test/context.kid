<?xml version='1.0' encoding='utf-8'?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt><p py:content="foo">should be 10</p></attempt>
    <expect><p>10</p></expect>
  </test>
  <test>
    <attempt><p py:content="bar">should be bla bla</p></attempt>
    <expect><p>bla bla</p></expect>
  </test>
</testdoc>
