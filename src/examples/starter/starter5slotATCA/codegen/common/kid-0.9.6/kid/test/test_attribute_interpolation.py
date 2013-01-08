"""Unit Tests for expression substition in attributes."""

__revision__ = "$Rev: 428 $"
__author__ = "Christoph Zwerschke <cito@online.de>"
__copyright__ = "Copyright 2006, Christoph Zwerschke"

import kid


def test_simple_interpolation():
    """Simple variable substitution."""
    s = kid.Template('<div x="$a $b $c" />',
        a='hello', b=123, c=0.5).serialize()
    assert s.endswith('<div x="hello 123 0.5" />')

def test_expression_interpolation():
    """Expression substitution."""
    s = kid.Template('<div x="${a+b+c}" />',
        a=1, b=2, c=3).serialize()
    assert s.endswith('<div x="6" />')

def test_remove_null_attributes():
    """Expressions evaluating to None should remove the attribute."""
    source = '<div a="$a" b="$b" c="$c" />'
    s = kid.Template(source, a=0, b=False, c=None).serialize()
    assert s.endswith('<div a="0" b="False" />')
    source = '<div a="$c$a" b="$c$b" c="$c$c" />'
    s = kid.Template(source, a=0, b=False, c=None).serialize()
    assert s.endswith('<div a="0" b="False" />')

def test_interpolated_xml():
    """Harmless XML expressions should be allowed here (ticket #27)."""
    sources = ('<meta content="$xml" />',
        """<meta xmlns:py="http://purl.org/kid/ns#"
        content="temp" py:attrs="content=xml"/>""")
    from kid.template_util import TemplateAttrsError
    for source in sources:
        xml = kid.XML("<tag />")
        try:
            kid.Template(source, xml=xml).serialize()
        except TemplateAttrsError, e:
            e = str(e)
        else:
            e = 'tag accepted in attribute'
        assert e == 'Illegal value for attribute "content"'
        text = "R&#233;sultat de la requ&#234;te"
        xml = kid.XML(text)
        s = kid.Template(source, xml=xml).serialize(encoding='ascii')
        assert s.endswith('<meta content="%s" />' % text)
