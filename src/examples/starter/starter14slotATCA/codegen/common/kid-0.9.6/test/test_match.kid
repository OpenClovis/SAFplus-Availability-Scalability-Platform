<?xml version='1.0' encoding='utf-8'?>
<?python

from kid.namespace import Namespace
test = Namespace('urn:test')

?>
<testdoc xmlns:py="http://purl.org/kid/ns#" py:extends="'test_match_parent.kid'">
  

  <div py:match="item.tag == 'bar'" class="bar">
    ${[item.text, item.getchildren()]}
  </div>

  <test>
    <attempt>
      <bar>
        This <b>content</b> will be copied.
      </bar>
    </attempt>
    <expect type="text"><![CDATA[
      <div class="bar">
        This <b>content</b> will be copied.
      </div>
    ]]></expect>
  </test>
  
  <!-- simple test -->
  <div py:match="item.tag == 'foo'">
    <p>Foo: ${item.get('bar')}</p>
  </div>
  
  <foobar py:match="item.tag == 'foobar'" class="foobar">first parent - ${[item.text, item.getchildren()]}
  </foobar>
  

  <test>
    <attempt><foo bar="baz" /></attempt>
    <expect type="text"><![CDATA[
      <div>
        <p>Foo: baz</p>
      </div>]]></expect>
  </test>

  <!-- recursive apply test -->
  <div py:match="item.tag == 'baz'" class="baz">
    ${apply(item.getchildren())}
  </div>
  
  <test>
    <attempt>
      <baz>
        <foo bar="bling"/>
        <bar>bla bla</bar>
      </baz>
    </attempt>
    <expect type="text"><![CDATA[
      <div class="baz">
        <div>
          <p>Foo: bling</p>
        </div>
        <div class="bar">
          bla bla
        </div>
      </div>
    ]]></expect>
  </test>


</testdoc>
