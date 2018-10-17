# -*- coding: utf-8 -*-

"""Kid tranformations"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

from kid.parser import ElementStream, START, XML_DECL, document, _coalesce
from kid.namespace import Namespace
from kid.template_util import generate_content

__all__ = ['transform_filter']


def transform_filter(stream, template):
    templates = template._get_match_templates()
    def apply_func(item):
        return transform_filter(generate_content(item), template)
    stream = ElementStream.ensure(stream)
    return ElementStream(apply_matches(stream, template, templates, apply_func))

def apply_matches(stream, template, templates, apply_func):
    for ev, item in stream:
        if ev == START:
            matched = False
            for i in range(0, len(templates)):
                match, call = templates[i]
                if match(item):
                    item = stream.expand()
                    newstream = _coalesce(call(template, item, apply_func),
                        template._get_assume_encoding())
                    if len(templates) < 2:
                        for ev, item in newstream:
                            yield ev, item
                    else:
                        for ev, item in apply_matches(
                                ElementStream(newstream), template,
                                templates[:i] + templates[i+1:], apply_func):
                            yield ev, item
                    matched = True
                    break
            if matched:
                continue
        yield ev, item

# XXX haven't tested this yet..
def xinclude_filter(stream, template):
    xi = Namespace('http://www.w3.org/2001/XInclude')
    include = xi.include
    fallback = xi.fallback
    for ev, item in stream:
        if ev == START and item.tag == include:
            item = item.expand()
            href = item.get('href')
            try:
                doc = document(href, template._get_assume_encoding())
            except:
                fallback_elm = item.find(fallback)
                for ev, item in ElementStream(fallback_elm).strip(1):
                    yield ev, item
            else:
                for ev, item in doc:
                    if ev != XML_DECL:
                        yield ev
