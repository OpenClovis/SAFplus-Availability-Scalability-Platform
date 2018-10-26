

__revision__ = "$Rev: 421 $"
__author__ = "David Stanek <dstanek@dstanek.com>"
__copyright__ = "Copyright 2005, David Stanek"

import kid

def test_strip_no_expr():
    """A py:strip without an expression will strip that element."""
    source = """
        <test xmlns:py="http://purl.org/kid/ns#">
            <wrapper py:strip="">
                <present>stuff</present>
            </wrapper>
        </test>
    """
    data = kid.Template(source=source).serialize()
    assert "wrapper" not in data
    assert "present" in data

def test_strip_with_boolean_expression__or():
    """Test for the bug that was reported in ticket #97."""
    source_template = """
        <?python
        a = %s
        b = %s
        ?>
        <test xmlns:py="http://purl.org/kid/ns#">
            <el py:strip="(a or b)">content</el>
            <el py:strip="a or b">content</el>
        </test>
    """
    t = kid.Template(source=source_template % (True, True))
    assert "<el>" not in t.serialize()
    t = kid.Template(source=source_template % (True, False))
    assert "<el>" not in t.serialize()
    t = kid.Template(source=source_template % (False, True))
    assert "<el>" not in t.serialize()
    t = kid.Template(source=source_template % (False, False))
    assert t.serialize().count("<el>") == 2

def test_strip_with_boolean_expression__eq():
    source = """
        <test xmlns:py="http://purl.org/kid/ns#">
            <el0 py:strip="1==1" />
            <el1 py:strip="1==0" />
        </test>
    """
    data = kid.Template(source=source).serialize()
    assert "<el0" not in data
    assert "<el1" in data

def test_replace():
    source = """
        <test xmlns:py="http://purl.org/kid/ns#">
            <element py:replace="'x'">
                you will never see this
            </element>
        </test>
    """
    data = kid.Template(source=source).serialize()
    assert "wrapper" not in data
    assert "x" in data

def test_replace_with_strip():
    """py:strip as ignored if py:replace exists in the same element."""
    source = """
        <test xmlns:py="http://purl.org/kid/ns#">
            <element py:replace="'x'" py:strip="">
                content
            </element>
        </test>
    """
    data = kid.Template(source=source).serialize()
    assert "wrapper" not in data
    assert "x" in data

def test_attr():
    source = """
        <test xmlns:py="http://purl.org/kid/ns#">
            <elem py:attrs="{'a':1, 'ns:b':2}" />
            <elem py:attrs="'a':1, 'ns:b':2" />
            <elem py:attrs="(('a',1), ('ns:b',2))" />
            <elem py:attrs="a=1, ns:b=2" />
        </test>
    """
    data = kid.Template(source=source).serialize()
    assert data.count('<elem a="1" ns:b="2" />') == 4

def test_attr_with_strip():
    source = """
        <test xmlns:py="http://purl.org/kid/ns#">
            <element py:strip="False" py:attrs="a=1, b=2"/>
        </test>
    """
    data = kid.Template(source=source).serialize()
    print data
    assert 'a="1"' in data
    assert 'b="2"' in data
