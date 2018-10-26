<?xml version='1.0' encoding='utf-8'?>
<?python
# some fruits
fruits = ["apple", "orange", "kiwi", "M&M"]
?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt><p py:strip="">outer p <em>is</em> omitted</p></attempt>
    <expect>outer p <em>is</em> omitted</expect>
  </test>
  
  <test>
    <attempt><p py:strip="True">outer p <em>is</em> omitted</p></attempt>
    <expect>outer p <em>is</em> omitted</expect>
  </test>

  <test>
    <attempt><p py:strip="False">outer p is not omitted</p></attempt>
    <expect><p>outer p is not omitted</p></expect>
  </test>
</testdoc>
