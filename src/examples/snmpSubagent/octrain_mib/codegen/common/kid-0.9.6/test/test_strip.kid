<?xml version='1.0' encoding='utf-8'?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt><p py:strip="False">abc</p></attempt>
    <expect><p>abc</p></expect>
  </test>
  
  <test>
    <attempt><p py:strip="True">abc</p></attempt>
    <expect>abc</expect>
  </test>
  
  <test>
    <attempt><p py:strip="False"/></attempt>
    <expect><p/></expect>
  </test>
  
  <test>
    <attempt><p py:strip="True"/></attempt>
    <expect></expect>
  </test>
  
  <test>
    <attempt><p py:strip="">abc</p></attempt>
    <expect>abc</expect>
  </test>

  <test>
    <attempt><p py:strip="''">abc</p></attempt>
    <expect><p>abc</p></expect>
  </test>

  <test>
    <attempt><p py:strip="''">abc</p></attempt>
    <expect><p>abc</p></expect>
  </test>

  <test>
    <attempt><p py:replace="None" py:strip="False"/></attempt>
    <expect></expect>
  </test>
  
  <test>
    <attempt><p py:replace="None" py:strip="True"/></attempt>
    <expect></expect>
  </test>
  
  <test>
    <attempt><p py:content="None" py:strip="True"/></attempt>
    <expect></expect>
  </test>

  <test>
    <attempt><p py:content="None" py:strip="False"/></attempt>
    <expect><p/></expect>
  </test>

  <test>
    <attempt><p py:strip="False">a<b/>c</p></attempt>
    <expect><p>a<b/>c</p></expect>
  </test>
  
  <test>
    <attempt><p py:strip="True">a<b/>c</p></attempt>
    <expect>a<b/>c</expect>
  </test>

  <test>
    <attempt><p py:strip="0">abc</p></attempt>
    <expect><p>abc</p></expect>
  </test>
  
  <test>
    <attempt><p py:strip="1">abc</p></attempt>
    <expect>abc</expect>
  </test>

  <test>
    <attempt><p py:strip="[]">abc</p></attempt>
    <expect><p>abc</p></expect>
  </test>
  
  <test>
    <attempt><p py:strip="{}">abc</p></attempt>
    <expect><p>abc</p></expect>
  </test>
</testdoc>
