<?xml version='1.0' encoding='utf-8'?>
<?python #

import kid

some_text = "this is some text"
nada = None
some_xml = "<a><b><c>we'll stop here, thanks..</c></b></a>"
tmpl = kid.Template(source=some_xml)
def f(): return None

?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt><p py:replace="some_text">this should be replaced</p></attempt>
    <expect>this is some text</expect>
  </test>

  <test>
    <attempt><p py:replace="nada">this should go away</p></attempt>
    <expect></expect>
  </test>

  <test>
    <attempt><p py:replace="nada"><embedded /></p></attempt>
    <expect></expect>
  </test>

  <test>
    <attempt><p py:replace="f()">this should go away</p></attempt>
    <expect></expect>
  </test>

  <test>
    <attempt><p py:replace="XML(some_xml)">will be replaced</p></attempt>
    <expect><a><b><c>we'll stop here, thanks..</c></b></a></expect>
  </test>

  <test>
    <attempt><p py:replace="tmpl" /></attempt>
    <expect><a><b><c>we'll stop here, thanks..</c></b></a></expect>
  </test>
</testdoc>
