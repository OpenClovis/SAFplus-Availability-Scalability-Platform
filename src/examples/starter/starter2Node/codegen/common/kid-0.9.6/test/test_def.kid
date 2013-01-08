<?xml version='1.0' encoding='utf-8'?>
<?python #
from test.templates import template1, template2, template3

fruits = { 'apple' : 'A red fruit that hurts my teeth.',
           'orange' : 'A juicy fruit from Florida mostly.',
           "m&m's" : 'melts in your mouth not in your hand.' }

?>
<testdoc xmlns:py="http://purl.org/kid/ns#">

  <test>
    <attempt py:content="template1()" />
    <expect><test1><p>This is a test</p></test1></expect>
  </test>
  <test>
    <attempt py:content="template2('hello world?')" />
    <expect><test2><p>hello world?</p></test2></expect>
  </test>
  <test>
    <attempt py:content="template3('Some Fruit', fruits)" />
    <expect><test3>
    <h1>Some Fruit</h1>
    <dl>

      <dt>orange</dt>
      <dd>A juicy fruit from Florida mostly.</dd>

      <dt>m&amp;m's</dt>
      <dd>melts in your mouth not in your hand.</dd>

      <dt>apple</dt>
      <dd>A red fruit that hurts my teeth.</dd>

    </dl>
  </test3></expect>
  </test>

  <!-- define a template inline -->
  <p py:def="inline_test">a test paragraph</p>

  <test>
    <attempt py:content="inline_test()"/>
    <expect><p>a test paragraph</p></expect>
  </test>

  <test>
    <attempt>${ inline_test() } and then ${'some'}</attempt>
    <expect><p>a test paragraph</p> and then some</expect>
  </test>

  <p py:def="hello_world(x='World')" py:strip="">${x}</p>

  <test>
    <attempt><p py:replace="hello_world(x='Hello')"/>,
    <p py:replace="hello_world()"/>!</attempt>
    <expect>Hello,
    World!</expect>
  </test>

  <!-- XXX need a recursive test -->

</testdoc>
