import pdb

class Dot:
  """
  The dot class lets you access fields either through field notation (.)
  or dictionary notation ([]).
  """

  def __init__(self, dict={}):
    for (key, val) in dict.items():
      if type(val) == type({}): val = Dot(val)
      self.__dict__[key] = val

  def __str__(self):
    s=[]
    for (key, val) in self.__dict__.items():
      s.append("'%s':%s" % (str(key), str(val)))
    return "Dot{ " + ", ".join(s) + " }"

  def __repr__(self):
    return 'Dot(%s)' % self.__str__()

  def has_key(self, key):
    return self.__dict__.has_key(key)

  def keys(self):
    return self.__dict__.keys()

  def values(self):
    return self.__dict__.values()

  def items(self):
    return self.__dict__.items()

  def clear(self):
    self.__dict__.clear()

  def get(self, key, default = None):
    return self.__dict__.get(key, default)

  def setdefault(self, key, default = None):
    return self.__dict__.setdefault(key, default)

  def pop(self, key, default = None):
    return self.__dict__.pop(key, default)

  def popitem(self):
    return self.__dict__.popitem()

  def iteritems(self):
    return self.__dict__.iteritems()

  def iterkeys(self):
    return self.__dict__.iterkeys()

  def itervalues(self):
    return self.__dict__.itervalues()

  def update(self,d):
    for (k,v) in d.items():
      self.__setitem__(k,v)

  def __getitem__(self, key):
    try:
      i = key.index('.')
      return self.__dict__[key[:i]][key[i+1:]]
    except ValueError:  # There is no '.' in the key
      return self.__dict__[key]
    except AttributeError: # key is not a string
      return self.__dict__[key]

  def __setitem__(self, key, val):
    try: # strip unicode, if val is a string
      val = val.encode('utf8')
    except:
      pass

    try:
      i = key.index('.')
      key_1 = key[:i]
      key_rest = key[i+1:]
      if not self.__dict__.has_key(key_1):
        self.__dict__[key_1] = Dot()
      self[key_1][key_rest] = val
    except ValueError:
      self.__dict__[key] = val

  def __delitem__(self, key):
    del self.__dict__[key]

  def __len__(self):
    return self.__dict__.__len__()

  def __contains__(self, item):
    return item in self.__dict__

  def flatten(self, sep=".",prefix=None):
    """return a non-nested dictionary which contains all the elements of a nested dictionary, but the keys have been transformed to contain the nesting hierarchy.  For example {'a':{'b':'1','c':'2'}} -> {'a.b':'1','a.c':'2'}
    @param  sep (Optional) The separator to use between keys.  Default is .
    @param  prefix (Optional) A string to prepend to each key
    """
    d = {}
    for (key, val) in self.__dict__.items():
      p = (prefix and prefix+sep or '')+str(key)
      try:
        d.update(val.flatten(p,sep))
      except AttributeError:
        d[p] = val
    return d

#
# unittest - run 'python <filename> to execute it
#
if __name__ == '__main__':
    import unittest
    class test(unittest.TestCase):
        d = Dot({'a': 12, 'b':{'c':4, 'd':16}})
        # These tests are interdependent, relying on the order of execution!
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
            d = Dot(self.d)
            d.b.e.f.g = 12
            self.assertEqual(d.__str__(),
                "{ 'a':12, 'b':{ 'c':4, 'e':{ 'f':{ 'g':12 } }, 'd':16 } }")

    unittest.main()
