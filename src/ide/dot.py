import pdb
from types import *

def findCfg(key,cfgLst,default=None):
  for a in cfgLst:
    if key in a: return a[key]
  return default

class Dot:
  """
  The dot class lets you access fields either through field notation (.)
  or dictionary notation ([]).
  """

  def shouldRecurse(self,thing):
    return (type(thing) == type({})) or  (type(thing) is object and thing.__class__ is Dot)

  def __init__(self, coll={}):
    self.__dict__["_order"]=[]
    if type(coll) is dict:
      for (key, val) in list(coll.items()):
        if self.shouldRecurse(val): val = Dot(val)
        self._set(key,val)
    elif type(coll) is list:
      for (key, val) in coll:
        if self.shouldRecurse(val): val = Dot(val)
        self._set(key,val)
    elif type(coll) is object and coll.__class__ is Dot:
      for (key,val) in coll.items():
        if self.shouldRecurse(val): val = Dot(val)
        self._set(key,val)
      
      
  def __str__(self):
    s=[]
    for key in self._order:
      val = self.__dict__[key]
      #for (key, val) in self.__dict__.items():
      
      if type(val) in StringTypes:
        val = "'" + val + "'"
      s.append("'%s':%s" % (str(key), str(val)))
    return "{ " + ", ".join(s) + " }"

  def _set(self,k,v):
    if k not in self.__dict__:
      self._order.append(k)
    self.__dict__[k] = v
    return v

  def __repr__(self):
    return 'Dot(%s)' % self.__str__()

  def merge(self,dict):
    """Like dictionary update but subdictionaries are recursively merged"""
    for (k,v) in list(dict.items()):
      if type(v) == object and v.__class__ == Dot and k in self.__dict__:  # If dict and I both have a same-named subDOT then merge them
        self.__dict__[k].merge(v)
      else:
        self._set(k,v)

  def find(self, key):
    """Do a recursive search for the key, returning the complete path"""
    if key in self: return key
    for (k,v) in self.items():
      if type(v) == object and v.__class__ == Dot:
        r = v.find(key)
        if r: return ".".join([k,r])
    return None
          
  def update(self,dict):
    """Update is like merge but it will not merge subdictionaries -- it overwrites them"""
    for (k,v) in list(dict.items()):
      self._set(k,v)

  def has_key(self, key):
    return key in self.__dict__

  def keys(self):
    return self._order

  def values(self):
    return [self.__dict__[x] for x in self._order]

  def items(self):
    return [(x,self.__dict__[x]) for x in self._order]

  def clear(self):
    self.__dict__.clear()
    self._order = []

  def get(self, key, default = None):
    return self.__dict__.get(key, default)

  def setdefault(self, key, default = None):
    if key in self.__dict__: return self.__dict__[key]
    self._set(key,default)
    return default

  def pop(self, key, default = None):
    try:
      self._order.remove(key)
    except ValueError:  # Key not found
      pass
    return self.__dict__.pop(key, default)

  def popitem(self):
    key = self._order.pop()
    ret = (key,self.__dict__[key])
    del self.__dict__[key]
    return ret

  def iteritems(self):
    for key in self._order:
      yield (key,self.__dict__[key])
    #return self.__dict__.iteritems()

  def iterkeys(self):
    for key in self._order: yield key

  def itervalues(self):
    for key in self._order:
      yield self.__dict__[key]

  def __getitem__(self, key):
    """ Getitem is different then the standard dictionary because it interprets . to mean delve into a sub-dictionary"""
    try:
      i = key.index('.')
      return self.__dict__[key[:i]][key[i+1:]]
    except ValueError:
      return self.__dict__[key]
    except AttributeError:  # key is not a string (could be None for example)
      return self.__dict__[key]      

  def __setitem__(self, key, val):
    """ Setitem is different then the standard dictionary because it interprets . to mean delve into a sub-dictionary"""
    try:
      i = key.index('.')
      key_1 = key[:i]
      key_rest = key[i+1:]
      if key_1 not in self.__dict__:
        self._set(key_1,Dot())
      self[key_1][key_rest] = val
    except ValueError:
      self._set(key,val)

  def __delitem__(self, key):
    del self.__dict__[key]
    self._order.remove[key]

  def __setattr__(self, item, value):
    return self._set(item,value)

  def __len__(self):
    return self.__dict__.__len__()

  def __contains__(self, item):
    return item in self.__dict__

  def flatten(self, prefix=None):
    d = {}
    for (key, val) in self.items():
      p = (prefix and prefix+'.' or '')+str(key)
      try:
        d.update(val.flatten(p))
      except AttributeError:
        d[p] = val
    return d

##
## unittest - run 'python <filename> to execute it
##
if __name__ == '__main__':
    import unittest
    import pdb
    class test(unittest.TestCase):
        # These tests are interdependent, relying on the order of execution!
        d = Dot({'a': 12, 'b':{'c':4, 'd':16}})
        def test_01_init(self): d = Dot()
        def test_02_init(self): d = Dot({})
        def test_03_init(self): d = Dot({'a': 12})
        def test_04_init(self): d = Dot({'a': 12, 'b':{'c':4, 'd':16}})
        def test_05_insert_as_dict(self): d = Dot(); d['new']='new value'
        def test_06_insert_as_class(self): d = Dot(); d.new = 'new value'
        def test_07_insert_dot(self):
            d = Dot({'foo':3})
            d.new = Dot({'bar':2})
        def test_08_str(self):
            self.assertEqual(self.d.__str__(),
                "{ 'a':12, 'b':{ 'c':4, 'd':16 } }")
        def test_09_repr(self):
            self.assertEqual(self.d.__repr__(),
                "Dot({ 'a':12, 'b':{ 'c':4, 'd':16 } })")
        def test_20_flatten(self):
            self.assertEqual(Dot().flatten(), {})
        def test_21_flatten(self):
            self.assertEqual(Dot({'a':1}).flatten(), {'a':1})
        def test_22_flatten(self):
            self.assertEqual(self.d.flatten(), {'a': 12, 'b.c': 4, 'b.d': 16})
        def test_30_dotted_key_get(self):
            self.assertEqual(self.d['b.c'], 4)
        def test_31_dotted_key_get(self):
            self.assertEqual(self.d.b.c, 4)
        def test_32_dotted_key_set(self):
            d = Dot(self.d)
            d['b.e.f.g'] = 12
            self.assertEqual(d.__str__(),
                "{ 'a':12, 'b':{ 'c':4, 'e':{ 'f':{ 'g':12 } }, 'd':16 } }")
        def test_33_dotted_key_set(self):
            # pdb.set_trace()
            d = Dot(self.d)
            d.b.e.f.g = 12
            print(str(d))
            self.assertEqual(d.__str__(),
                "{ 'a':12, 'b':{ 'c':4, 'e':{ 'f':{ 'g':12 } }, 'd':16 } }")

    unittest.main()
