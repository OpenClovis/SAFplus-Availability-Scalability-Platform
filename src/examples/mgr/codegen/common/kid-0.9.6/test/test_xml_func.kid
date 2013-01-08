<?xml version='1.0' encoding='utf-8'?>

<?python def test_xml_global(): return XML('<a/><b/><c/>') ?>


<testdoc xmlns:py="http://purl.org/kid/ns#">

  <?python
  test_xml_local = test_xml_global
  # XXX: Uncommenting the following line should work, but doesn't.
  # (Results in an "unqualified exec is not allowed here" error.)
  # def test_xml_local(): return XML('<a/><b/><c/>')
  ?>

  <d py:def="test_xml_template()"><a/><b/><c/></d>

  <test>
    <attempt><r py:replace="test_xml_global()">abc</r></attempt>
    <expect><a/><b/><c/></expect>
  </test>

  <test>
    <attempt>${test_xml_global()}</attempt>
    <expect><a/><b/><c/></expect>
  </test>

  <test>
    <attempt><r py:replace="test_xml_local()">abc</r></attempt>
    <expect><a/><b/><c/></expect>
  </test>

  <test>
    <attempt>${test_xml_local()}</attempt>
    <expect><a/><b/><c/></expect>
  </test>

  <test>
    <attempt><r py:replace="test_xml_template()">abc</r></attempt>
    <expect><d><a/><b/><c/></d></expect>
  </test>

  <test>
    <attempt>${test_xml_template()}</attempt>
    <expect><d><a/><b/><c/></d></expect>
  </test>

</testdoc>
