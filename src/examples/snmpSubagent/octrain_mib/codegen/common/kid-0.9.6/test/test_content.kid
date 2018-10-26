<?xml version='1.0' encoding='utf-8'?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
                      "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<?python

import kid

some_text = "this is some text"
def some_function():
  return "this is some function"
nada = None
some_xml = "<a><b><c>we'll stop here, thanks..</c></b></a>"
tmpl = kid.Template(source=some_xml)

?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt py:content="some_text">test with a module-level variable</attempt>
    <expect>this is some text</expect>
  </test>

  <test>
    <attempt py:content="some_function()">test with a module-level function</attempt>
    <expect>this is some function</expect>
  </test>

  <test>
    <attempt py:content="">test with empty content expression</attempt>
    <expect></expect>
  </test>

  <test>
    <attempt py:content=""><embedded /></attempt>
    <expect></expect>
  </test>

  <test>
    <attempt py:content="nada">test with None</attempt>
    <expect></expect>
  </test>

  <test>
    <attempt py:content="nada"><embedded /></attempt>
    <expect></expect>
  </test>

  <test>
    <attempt py:content="'some text'">
      <p>
        <em>Here</em>'s some embedded structure too. This should not
        show up in the output document.
      </p>
      <p>
        This shouldn't either.
      </p>
    </attempt>
    <expect>some text</expect>
  </test>

  <test>
    <attempt py:content="1234">int test</attempt>
    <expect>1234</expect>
  </test>

  <test>
    <attempt py:content="1234.5678">float test</attempt>
    <expect>1234.5678</expect>
  </test>

  <test>
    <attempt py:content="'\xe2\x80\xa0\xc2\xa9\xe2\x80\x94'.decode('utf-8')">unicode test</attempt>
    <expect>†©—</expect>
  </test>

  <test>
    <attempt><![CDATA[quick < CDATA test]]></attempt>
    <expect>quick &lt; CDATA test</expect>
  </test>

  <test>
    <!-- Check that xml:* attributes pass through properly. -->
    <attempt><elm xml:lang="bla"/></attempt>
    <expect><elm xml:lang="bla"/></expect>
  </test>

  <?python
     msg      = u'ca\xe7\xe3o'
     msg_utf8 =  'ca\xc3\xa7\xc3\xa3o'
     msg_iso  =  'ca\xe7\xe3o'
     msg2     = '\xe2'
  ?>

  <test>
    <attempt py:content="msg"></attempt>
    <expect>cação</expect>
  </test>
  <test>
    <attempt py:content="unicode(msg_utf8, 'utf-8')"></attempt>
    <expect>cação</expect>
  </test>
  <test>
    <attempt py:content="unicode(msg_iso, 'iso8859-1')"></attempt>
    <expect>cação</expect>
  </test>

  <test>
    <attempt>wrapped with ${unicode(msg2, 'iso8859-1')} other text</attempt>
    <expect>wrapped with &#xe2; other text</expect>
  </test>

  <test>
    <attempt py:content="defined('fizzyfish')"></attempt>
    <expect>False</expect>
  </test>

  <test>
    <attempt>${value_of("fizzyfish", "extra fizzy")}</attempt>
    <expect>extra fizzy</expect>
  </test>

  <test>
    <attempt><p py:content="tmpl" /></attempt>
    <expect><p><a><b><c>we'll stop here, thanks..</c></b></a></p></expect>
  </test>
</testdoc>
