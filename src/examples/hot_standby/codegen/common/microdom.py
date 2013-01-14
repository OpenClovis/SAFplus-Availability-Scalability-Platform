# Original:
# Copyright (C) 2004 G. Andrew Stone
# This software is released into the Public Domain
#
###############################################################################
#
# Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
# 
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is  free software; you can redistribute it and / or
# modify  it under  the  terms  of  the GNU General Public License
# version 2 as published by the Free Software Foundation.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You  should  have  received  a  copy of  the  GNU General Public
# License along  with  this program. If  not,  write  to  the 
# Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
###############################################################################

import pdb
from types import *
import xml.dom.minidom
from common import *

Truths =["CL_TRUE","cl_true","TRUE","true","True"]

Falses =["CL_FALSE","cl_false","FALSE","false","False"]

def keyify(s):
  s = s.replace(":","_")
  return s

class AnyChild:
  def __init__(self, microdom,recurse=False):
    self.microdom = microdom
    self.recurse = recurse

  def __getattr__(self,item):
    for c in self.microdom.children_:
      try:
        result = c[item]
        if result is not None: return result
      except:
        pass
    for (k,v) in self.microdom.attributes_.items():
      try:        
        if k == item: return v        
      except:
        pass

    if self.recurse:
      # Look for a tag
      result = self.microdom.filterByAttr({"tag_":item})
      if result: return result[0]
      # Look for an attribute
      result = self.microdom.filterByAttr({item:lambda x: not x is None})
#      print "attr:",result
      if result: 
        return result[0].attributes_[item]

    raise AttributeError(item)


  

class MicroDom:
  def __init__(self, attributes,children,data):

    # formal access
    self.tag_        = attributes["tag_"]
    self.attributes_ = attributes
    self.children_   = children
    self.child_      = {}
    self.data_       = data

    self.anychild_   = AnyChild(self)
    self.any_        = AnyChild(self,True)
  
    # quick access
    for (k,v) in attributes.items():
      try:
        self.__dict__[keyify(k)] = v
      except:
        pass
    
    for c in children:
      try:
        self.__dict__[keyify(c.tag_)] = c
        self.child_[keyify(c.tag_)] = c
        if c.attributes_.has_key["id"]:
          self.child_[keyify(c.id)] = c
        if c.attributes_.has_key["name"]:
          self.child_[keyify(c.name)] = c
      except:
        pass
  
  def delve(self,lst,lstlen=0):
      if not lstlen: lstlen = len(lst)

      c = self.child_.get(lst[0],None)
      if c:
        if lstlen>1:
          return c.delve(lst[1:],lstlen-1)
        else: return c
      if lst[0] == "any_":
        for c in self.children_:
          result = c.delve(lst[1:],lstlen-1)
          if not result is None: return result  # Return the first match

      return None
  
  def children(self,filter = None):
    """Return all children of type MicroDom by default, or return the transformation function applied to all children, eliminating any that return None"""
    ret = []
    for c in self.children_:
      if filter is not None:
        t = filter(c)
        if t is not None: ret.append(t)
      else:
        if isInstanceOf(c,MicroDom): ret.append(c)
    return ret

  def get(self,item,d=None):
    try:
      d = self.__getitem__(item)
    except KeyError:
      pass
    return d

  def __getitem__(self,item):
    #print "ITEM: %s" % item
    if self.tag_ == item: return self.data_
    t = self.attributes_.get(item,None)
    if not t is None: return t
    t = self.child_.get(item,None)
    if not t is None: return t

    ilst = item.split(".")
    if len(ilst)>1: 
      ret = self.delve(ilst)
      if ret is not None: return ret
    raise KeyError(item)


  def attrMatch(self,attrs,missingIsOk=False):
    """
    For attributes you can pass in a dictionary.  The value can be either an item, or a function.  If a function the function is called with this class's attribute's value as an arguement.  The fn should return "False" if this class does NOT match.
    """
    for (k,v) in attrs.items():
      myval = self.attributes_.get(k,None)
      if type (v) is FunctionType:
        if not v(myval): return False
      elif myval is not None:
        if not v == myval: return False
      elif not missingIsOk: return False
    return True

  def filterByAttr(self,attrs):
    ret = []
    if self.attrMatch(attrs): ret.append(self)
    for c in self.children_:
      try:
        ret += c.filterByAttr(attrs)
      except AttributeError: # Its a leaf - ie. NOT a microdom object
        pass
    return ret

  def find(self,tagOrAttr):
    ret = self.findByAttr({"tag_":tagOrAttr})
    if not ret is None: return ret
    ret = self.findByAttr({tagOrAttr:lambda x: not x is None})
    if not ret is None: return ret
    return None


  def findByAttr(self,attrs,prefix=""):
    ret = []
    if self.attrMatch(attrs): ret.append((prefix+self.tag_,self))
    for c in self.children_:
      try:
        ret += c.findByAttr(attrs,prefix + self.tag_ + ".")
      except AttributeError: # Its a leaf - ie. NOT a microdom object
        pass
    return ret
      

  def getElementsByTagName(self,tag):
    return self.filterByAttr({"tag_":tag})

  def filterByTag(self,tag):
    return self.filterByAttr({"tag_":tag})

  def __repr__(self):
    return self.dump(True)

  def __str__(self):
    return self.dump(False)

  def dump(self, recurse=True):
    if self.data_ or self.children_:
      full = 1
    else: full = 0

    if self.data_: datastr = str(self.data_)
    else: datastr = ""
    if self.children_:
      if not recurse: chstr = "<%d children>" % len(self.children_)
      else:
        chlst = []
        for c in self.children_:
          try:
            chlst.append(c.dump())
          except AttributeError:
            chlst.append(str(c))
        chstr = "".join(chlst)
    else: chstr = ""

    if len(self.attributes_)>1:
      # Format attribute string, eliminating the tag_ attribute
      attrs = ["%s='%s'" % (k,v) for (k,v) in filter(lambda i: i[0] != 'tag_', self.attributes_.items())]
      attrs = " " + " ".join(attrs)
    else:
      attrs = ""

    if full:
      return "<%s%s>%s%s</%s>" % (self.tag_,attrs,datastr,chstr,self.tag_)
    else:
      return "<%s%s />" % (self.tag_,attrs)


def LoadFile(fil):
  if type(fil) is ListType:
    return [LoadFile(f) for f in fil]

  dom = xml.dom.minidom.parse(fil)
  return LoadMiniDom(dom.childNodes[0])


def LoadMiniDom(dom):
  if type(dom) is ListType:
    return [LoadMiniDom(d) for d in dom]

  childlist = []
  attrlist = { "tag_": dom.nodeName}
  dataLst=[]

  if dom.childNodes:
    for child in dom.childNodes:
      if child.nodeName == '#text' or child.nodeName == '#cdata-section':  # Raw data
        childlist.append(child.nodeValue)
        dataLst.append(str(child.nodeValue))
      else:
        childlist.append(LoadMiniDom(child))

  for (key,val) in dom._attrs.items():
    attrlist[key] = val.value

  try: 
      data = dom.data
  except AttributeError:
      data = "".join(dataLst) 

  try:
    data = int(data)
    goodAsInt = True
  except ValueError:
    try:
      data = float(data)    
    except ValueError:
      data = str(data)
      if data in Truths: data = True
      if data in Falses: data = False
  
  return MicroDom(attrlist, childlist, data)


def Merge(topTag,children):
  """Take a list of DOMs and put them all under one top tag"""
  return MicroDom({"tag_":topTag},children,None)


def MicroDom2Dict(listofDoms, keyXlat=None, valueXlat=None, recurse=None):
  """Take a list of Doms and transform them into a dictionary using an overrideable key and value.
    By default the key will be the dom tag, and the value the node
    But you can pass in transformation functions that take the node as a parameter and return a key.  If you don't want this node in the dictionary, return None
    ditto for value and recurse"""
  ret = {}
  if keyXlat is None: keyXlat = lambda x: x.tag_
  if valueXlat is None: valueXlat = lambda x: x
  for n in listofDoms:
    k = keyXlat(n)
    if k is not None:
      ret[keyXlat(n)] = valueXlat(n)
    if recurse and recurse(n):
      ret.update(MicroDom2Dict(n.children_,keyXlat,valueXlat,recurse))

  return ret

def Test():
  
  x = """<sgType name="SAFComponent1SG">
         <failbackOption>CL_FALSE</failbackOption>
         <restartDuration>10000</restartDuration>
         <restartDuration2> 10000</restartDuration2>
         <test>dkekdae dkekd</test>
         </sgType>"""

  dom = xml.dom.minidom.parseString(x)
  md = LoadMiniDom(dom.childNodes[0])

  print md.restartDuration.data_
  print "%d" % md.restartDuration2.data_
  print md.failbackOption.data_
  print md.test.data_
  
