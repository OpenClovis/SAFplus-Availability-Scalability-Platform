import pickle
import defaults
import util

class InvalidSettingError(util.BaseException):
    pass
    
class NOT_SET(object):
    pass
    
class Settings(object):
    def __init__(self, parent):
        self._parent = parent
    def __getattr__(self, name):
        if name.startswith('_'):
            return super(Settings, self).__getattr__(name)
        value = self.get(name)
        if value != NOT_SET:
            return value
        if self._parent:
            return getattr(self._parent, name)
        raise InvalidSettingError, 'Invalid setting: %s' % name
    def __setattr__(self, name, value):
        if name.startswith('_'):
            super(Settings, self).__setattr__(name, value)
            return
        if self.set(name, value):
            return
        if self._parent:
            setattr(self._parent, name, value)
            return
        raise InvalidSettingError, 'Invalid setting: %s' % name
    def get(self, name):
        raise NotImplementedError, 'Settings subclasses must implement the get() method.'
    def set(self, name, value):
        raise NotImplementedError, 'Settings subclasses must implement the set() method.'
        
class ModuleSettings(Settings):
    def __init__(self, parent, module):
        super(ModuleSettings, self).__init__(parent)
        self._module = module
    def get(self, name):
        module = self._module
        if hasattr(module, name):
            return getattr(module, name)
        return NOT_SET
    def set(self, name, value):
        return False
        
class FileSettings(Settings):
    def __init__(self, parent, file):
        super(FileSettings, self).__init__(parent)
        self._file = file
        self.load()
    def load(self):
        try:
            input = open(self._file, 'rb')
            self._settings = pickle.load(input)
            input.close()
        except:
            self._settings = {}
    def save(self):
        output = open(self._file, 'wb')
        pickle.dump(self._settings, output, -1)
        output.close()
    def get(self, name):
        if name in self._settings:
            return self._settings[name]
        return NOT_SET
    def set(self, name, value):
        if value != getattr(self, name):
            self._settings[name] = value
            self.save()
        return True
        
class ProxySettings(Settings):
    def __init__(self, parent, target=None):
        super(ProxySettings, self).__init__(parent)
        self._target = target
    def set_target(self, target):
        self._target = target
    def get_target(self):
        return self._target
    def get(self, name):
        target = self._target
        if target:
            return getattr(target, name)
        else:
            return NOT_SET
    def set(self, name, value):
        target = self._target
        if target:
            setattr(target, name, value)
            return True
        else:
            return False
            
class MemorySettings(Settings):
    def __init__(self, parent):
        super(MemorySettings, self).__init__(parent)
        self._data = {}
    def get(self, name):
        if name in self._data:
            return self._data[name]
        return NOT_SET
    def set(self, name, value):
        # TODO: should set() write to the lowest-level object that allows it?
        if self._parent:
            try:
                if self._parent.set(name, value):
                    return True
            except:
                pass
        self._data[name] = value
        return True
        
def create_chain():
    settings = ModuleSettings(None, defaults)
    settings = FileSettings(settings, 'settings.dat')
    #settings = MemorySettings(settings)
    #settings = ProxySettings(None, settings)
    return settings
    
settings = create_chain()


