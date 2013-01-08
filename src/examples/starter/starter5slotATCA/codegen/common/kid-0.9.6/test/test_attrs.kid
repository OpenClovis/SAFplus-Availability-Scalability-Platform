<?xml version='1.0' encoding='utf-8'?>
<?python
s = 'hello'
f = 123.456
?>
<testdoc xmlns:py="http://purl.org/kid/ns#" xmlns:my="my">

  <test>
    <attempt>
      <p py:attrs="{'a':1, 'b':2}" />
    </attempt>
    <expect>
      <p a="1" b="2" />
    </expect>
  </test>

  <test>
    <attempt>
      <p py:attrs="(('a', 1), ('b', 2))" />
    </attempt>
    <expect>
      <p a="1" b="2" />
    </expect>
  </test>

  <test>
    <attempt>
      <p py:attrs="a=1, b=2" />
    </attempt>
    <expect>
      <p a="1" b="2" />
    </expect>
  </test>

  <test>
    <attempt>
      <p x="not 10" y="one" z="two" py:attrs="g=f, s=s, x=10, y=None, z=''">
        harmless attrs test
      </p>
    </attempt>
    <expect>
      <p g="123.456" s="hello" x="10" z="">
        harmless attrs test
      </p>
    </expect>
  </test>

  <test>
    <attempt>
      <p class="j0" if="1" py:attrs='class=complex(real=0), if=2, return="3", del=None, yield=""'>
        offensive attrs test with Python keywords
      </p>
    </attempt>
    <expect>
      <p class="0j" if="2" return="3" yield="">
        offensive attrs test with Python keywords
      </p>
    </expect>
  </test>

  <test>
    <attempt>
      <p py:attrs="attrs=1, my:attrs=2">
        attrs test with qualified names
      </p>
    </attempt>
    <expect>
      <p attrs="1" my:attrs="2">
        attrs test with qualified names
      </p>
    </expect>
  </test>

</testdoc>
