"""Unicode tests"""

from kid.parser import to_unicode

astr = '\xe2\x80\xa0\xc2\xa9\xe2\x80\x94'
ustr = astr.decode('utf-8')

def test_to_unicode():
    assert to_unicode(ustr, 'utf-8') == ustr
    assert to_unicode(astr, 'utf-8') == ustr

    class C(object):
        def __unicode__(self):
            return ustr

    assert to_unicode(C(), 'utf-8') == ustr
