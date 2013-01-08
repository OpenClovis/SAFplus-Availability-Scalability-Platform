<?xml version='1.0' encoding='utf-8'?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
                      "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<?python
test1 = 'test1 value'
def test2():
  return 'test2 value'
?>
<testdoc xmlns:py="http://purl.org/kid/ns#">

  <test>
    <attempt>${test1}</attempt>
    <expect>test1 value</expect>
  </test>

  <test>
    <attempt>${test2()}</attempt>
    <expect>test2 value</expect>
  </test>

  <test>
    <attempt>${None}</attempt>
    <expect></expect>
  </test>

  <test>
    <attempt>${'test4'}</attempt>
    <expect>test4</expect>
  </test>

  <test>
    <attempt>${1234}</attempt>
    <expect>1234</expect>
  </test>

  <test>
    <attempt>${1234.5678}</attempt>
    <expect>1234.5678</expect>
  </test>

  <test>
    <attempt>${[1,2,3]}</attempt>
    <expect>123</expect>
  </test>

  <test>
    <attempt>${(4,5,6)}</attempt>
    <expect>456</expect>
  </test>

  <test>
    <attempt>${u'†©—'}</attempt>
    <expect>†©—</expect>
  </test>

  <test>
    <attempt><![CDATA[${'quick < CDATA test'}]]></attempt>
    <expect>quick &lt; CDATA test</expect>
  </test>

  <test>
    <attempt>hello ${'there'} this is ${None} a multi-part $test1</attempt>
    <expect>hello there this is  a multi-part test1 value</expect>
  </test>

  <test>
    <attempt>test $$escaping of $$ signs.</attempt>
    <expect>test $$escaping of $$ signs.</expect>
  </test>

  <test>
    <attempt>${1+2+3}pack</attempt>
    <expect>6pack</expect>
  </test>

  <test>
    <attempt>em${}p${   }ty</attempt>
    <expect>empty</expect>
  </test>

  <?python s = 'hello' ?>

  <test>
    <attempt>$s ${'world'}</attempt>
    <expect>${s + ' world'}</expect>
  </test>

  <test>
    <attempt>$$ $$ $$ $ $ $ $$ $$ $$</attempt>
    <expect>${'$ $ $ $ $ $ $ $ $'}</expect>
  </test>

  <test>
    <attempt>$$s $s $$$s $$$</attempt>
    <expect>${'$s hello $hello $$'}</expect>
  </test>

  <test>
    <attempt>es ist schon ${int(str(6-1))} vor ${3
        + 4
        + 5} aber ${s.replace('e', 'a')}!</attempt>
    <expect>es ist schon 5 vor 12 aber hallo!</expect>
  </test>

  <?python
  msg      = u'ca\xe7\xe3o'
  msg_utf8 =  'ca\xc3\xa7\xc3\xa3o'
  msg_iso  =  'ca\xe7\xe3o'
  ?>

  <test>
    <attempt>${msg}</attempt>
    <expect>cação</expect>
  </test>
  <test>
    <attempt>${unicode(msg_utf8, 'utf-8')}</attempt>
    <expect>cação</expect>
  </test>
  <test>
    <attempt>${unicode(msg_iso, 'iso8859-1')}</attempt>
    <expect>cação</expect>
  </test>

</testdoc>
