<?xml version='1.0' encoding='utf-8'?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt py:content="'test'" />
    <expect>test</expect>
  </test>

  <test>
    <attempt><div py:omit=""><p>test</p></div></attempt>
    <expect><p>test</p></expect>
  </test>
</testdoc>
