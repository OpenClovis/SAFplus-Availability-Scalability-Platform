<?xml version='1.0' encoding='utf-8'?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt><p py:if="False">test1</p></attempt>
    <expect/>
  </test>
  <test>
    <attempt><p py:if="True">test1</p></attempt>
    <expect><p>test1</p></expect>
  </test>
</testdoc>
