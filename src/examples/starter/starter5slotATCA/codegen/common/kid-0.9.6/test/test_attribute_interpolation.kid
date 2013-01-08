<?xml version='1.0' encoding='utf-8'?>
<?python
s = 'hello'
i = 1
f = 123.456
?>
<testdoc xmlns:py="http://purl.org/kid/ns#">

  <test>
    <attempt><p x="$s">string test</p></attempt>
    <expect><p x="hello">string test</p></expect>
  </test>

  <test>
    <attempt><p x="$i">int test</p></attempt>
    <expect><p x="1">int test</p></expect>
  </test>

  <test>
    <attempt><p x="$f">float test</p></attempt>
    <expect><p x="123.456">float test</p></expect>
  </test>

  <test>
    <attempt><p x="${s} there ${f} and ${i} hehe">mixed test</p></attempt>
    <expect><p x="hello there 123.456 and 1 hehe">mixed test</p></expect>
  </test>

  <test>
    <attempt><p x="${None}" y="${''}">Remove and blank attr test</p></attempt>
    <expect><p y="">Remove and blank attr test</p></expect>
  </test>

  <test>
    <attempt><p x="$$$$$$">Escape test</p></attempt>
    <expect><p x="${'$$$'}">Escape test</p></expect>
  </test>

  <test>
    <attempt><p x="$ $ $">Escape test</p></attempt>
    <expect><p x="${'$ $ $'}">Escape test</p></expect>
  </test>

</testdoc>
