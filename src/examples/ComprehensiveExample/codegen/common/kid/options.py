# -*- coding: utf-8 -*-

"""Configuration API."""

import release
__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "David Stanek <dstanek@dstanek.com>"
__copyright__ = "Copyright 2006, David Stanek"
__license__ = release.license

__all__ = ['Options']


_prefix = "kid:"

class Options(object):

    def __init__(self, options={}, **extra):
        self._options = {}
        for name, value in options.items():
            self.set(name, value)
        for name, value in extra.items():
            self.set(name, value)

    def isset(self, name):
        """Returns True if a option exists or False if it doesn't.

        name: the option to check
        """
        return self._options.has_key(self.__prefix(name))

    def get(self, name, default=None):
        """Get the value of an option.

        name: the option to retrieve
        default: returned for non-existing properties, defaults to None
        """
        return self._options.get(self.__prefix(name), default)

    def set(self, name, value):
        """Set the value of an option.

        name: the option to set
        value: the value to be set for the option
        Returns the value passed in.
        """
        self._options[self.__prefix(name)] = value
        return value

    def remove(self, name):
        """Remove an option."""
        if self.isset(name):
            del self._options[self.__prefix(name)]

    def __getitem__(self, name):
        if not self.isset(name):
            raise KeyError, "no option %s" % name
        return self.get(name)

    def __setitem__(self, name, value):
        self.set(name, value)

    def __delitem__(self, name):
        if not self.isset(name):
            raise KeyError, "no option %s" % name
        self.remove(name)

    def __prefix(self, name):
        """Add the prefix if it does not already exist."""
        if not name.startswith(_prefix):
            name = _prefix + name
        return name
