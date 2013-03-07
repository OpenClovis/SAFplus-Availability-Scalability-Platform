
class BuildDep:
    """ Object to define a dependency
            * A dependency used during install phase and packaged in 3rd party tarball
    """

    def __init__(self, name=''):
        self.name           = name.strip()
        self.required       = True

        self.version        = ''                    # version required by installer
        self.installedver   = ''                    # version already installed on system

        self.pkg_name       = ''                    # name of package within 3rd party tarball

        self.ver_test_cmd   = None                    # a unix command which yeilds the 
                                                    # version of the dep currently installed

        self.build_cmds     = ['../configure',      # list of commands in order to install a given dep
                               'make',
                               'make install']

        self.force_install  = False
        self.use_build_dir  = True
        self.extract_install = False                # extract only
            
class RepoDep:
    """ Object to define a dependency
            * apt-get or yum dependency 
            * internet access required
    """
    
    def __init__(self, name,req=True):
        if type(name) is type(''):
          self.name           = name.strip()
        else: 
          self.name = name

        self.required       = req
        
