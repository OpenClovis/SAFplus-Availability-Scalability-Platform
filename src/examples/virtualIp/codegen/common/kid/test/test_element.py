# -*- coding: utf-8 -*-

"""kid.element tests"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "David Stanek <dstanek@dstanek.com>"
__copyright__ = "Copyright 2006, David Stanek"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"


import re
from kid.element import Element, SubElement, \
    Comment, ProcessingInstruction, Fragment, QName, \
    namespaces, encode_entity


def test_element_init():
    element = Element("test0")
    assert element.tag == "test0"
    print element.attrib
    assert element.attrib == {}
    assert element.text is None
    assert element.tail is None

    element = Element("test1", dict(a=0, b=1))
    assert element.tag == "test1"
    assert element.attrib == {"a": 0, "b": 1}
    assert element.text is None
    assert element.tail is None

    element = Element("test2", a=0, b=1)
    assert element.tag == "test2"
    assert element.attrib == {"a": 0, "b": 1}
    assert element.text is None
    assert element.tail is None

    element = Element("test3", dict(a=0, b=1), c=2)
    assert element.tag == "test3"
    assert element.attrib == {"a": 0, "b": 1, "c": 2}
    assert element.text is None
    assert element.tail is None

def test_element_repr():
    element = Element("test0")
    print `element`
    match = re.match(r"^<Element test0 at [\-a-f\d]*>$", `element`)
    assert match is not None

def test_element_children():
    parent = Element("parent")
    parent.append(Element("child0"))
    parent.append(Element("child1"))
    parent.append(Element("child2"))
    parent.append(Element("child3"))

    assert len(parent) == 4
    assert parent[0].tag == "child0"
    assert parent[3].tag == "child3"

    parent[3] = Element("child4")
    assert parent[3].tag == "child4"

    del parent[3]
    assert len(parent) == 3

    child0, child1 = parent[:2]
    assert child0.tag == "child0"
    assert child1.tag == "child1"

    parent[:2] = Element("firstone"), Element("firsttwo")
    assert parent[0].tag == "firstone"
    assert parent[1].tag == "firsttwo"

    del parent[:2]
    assert len(parent) == 1
    print parent[0].tag
    assert parent[0].tag == "child2"

    parent.insert(0, Element("insert1"))
    parent.insert(0, Element("insert0"))
    assert parent[0].tag == "insert0"
    assert parent[1].tag == "insert1"

    assert len(parent) == 3
    child = parent[-1]
    parent.remove(child)
    assert len(parent) == 2

    children = parent.getchildren()
    assert len(children) == 2
    assert children[0].tag == "insert0"
    assert children[1].tag == "insert1"

    parent.clear()
    assert parent.tag == "parent"
    assert len(parent) == 0

def test_element_attributes():
    element = Element("test", dict(a=0, b=1), c=2)
    assert element.get("d") is None
    assert element.get("d", "") == ""
    assert element.get("c") == 2
    assert element.get("c", "") == 2

    element.set("c", "c")
    assert element.get("c") == "c"

    keys = element.keys()
    assert len(keys) == 3
    for k in ["a", "b", "c"]:
        assert k in keys

    items = element.items()
    assert len(items) == 3
    for i in [("a", 0), ("b", 1), ("c", "c")]:
        assert i in items

def test_subelement():
    parent = Element("parent")

    child0 = SubElement(parent, "child0")
    assert parent[0].tag == "child0"
    assert parent[0].attrib == {}
    assert parent[0].text is None
    assert parent[0].tail is None

    child1 = SubElement(parent, "child1", dict(a=0, b=1))
    assert parent[1].tag == "child1"
    assert parent[1].attrib == {"a": 0, "b": 1}
    assert parent[1].text is None
    assert parent[1].tail is None

    child2 = SubElement(parent, "child2", a=0, b=1)
    assert parent[2].tag == "child2"
    assert parent[2].attrib == {"a": 0, "b": 1}
    assert parent[2].text is None
    assert parent[2].tail is None

    child3 = SubElement(parent, "child3", dict(a=0, b=1), c=2)
    assert parent[3].tag == "child3"
    assert parent[3].attrib == {"a": 0, "b": 1, "c": 2}
    assert parent[3].text is None
    assert parent[3].tail is None

    assert parent.getchildren() == [child0, child1, child2, child3]

def test_comment():
    comment = Comment()
    assert comment.tag == Comment
    assert comment.text is None
    comment = Comment("some text here")
    assert comment.text == "some text here"

def test_processing_instruction():
    pi = ProcessingInstruction("test")
    assert pi.tag == ProcessingInstruction
    assert pi.text == "test"
    pi = ProcessingInstruction("test", "some text here")
    assert pi.text == "test some text here"

def test_fragment():
    fragment = Fragment()
    assert fragment.tag == Fragment
    assert fragment.text == ""
    fragment = Fragment("text here")
    assert fragment.text == "text here"

def test_qname():
    qname = QName("text")
    assert qname.text == "text"
    assert str(qname) == "text"
    qname = QName("urn:kid", "text")
    assert qname.text == "{urn:kid}text"
    assert str(qname) == "{urn:kid}text"

def test_namespaces():
    element = Element("test", {
        "xmlns": "default ns",
        "xmlns:junk-ns0": "junk-ns value 0",
        "xmlns:junk-ns1": "junk-ns value 1",
        "py:pyattr": "py value",
        "attr": "a new attr",
    })

    ns = namespaces(element)
    expected = {
        "": "default ns",
        "junk-ns0": "junk-ns value 0",
        "junk-ns1": "junk-ns value 1",
    }
    assert ns == expected
    assert element.get("xmlns") == "default ns"
    assert element.get("xmlns:junk-ns0") == "junk-ns value 0"
    assert element.get("xmlns:junk-ns1") == "junk-ns value 1"
    assert element.get("py:pyattr") == "py value"
    assert element.get("attr") == "a new attr"

    ns = namespaces(element, remove=True)
    assert element.get("xmlns") is None
    assert element.get("xmlns:junk-ns0") is None
    assert element.get("xmlns:junk-ns1") is None
    assert element.get("py:pyattr") == "py value"
    assert element.get("attr") == "a new attr"

def test_encode_entity():
    """TODO: do a more complete set of tests"""
    assert encode_entity("my text") == "my text"
    expected = "1 is &gt; 0 &amp;&amp; 2 &lt; 3"
    assert encode_entity("1 is > 0 && 2 < 3") == expected
