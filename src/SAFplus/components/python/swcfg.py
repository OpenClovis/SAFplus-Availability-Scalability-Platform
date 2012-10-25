"""@namespace swcfg
Software configuration.  Classes and functions that handle application, ASP, and OS configuration, compatibility, and dependencies.
"""

#from clusterinfo import *


def InServiceUpgradeable(cfg,clusterinfo=None):
  """ Can the cluster be upgraded to the specified configuration
  @param cfg A dictionary describing the available software in a bundle and its compatibility and dependencies.
  @return True if the cluster can be upgraded to the versions specified.  If the cluster cannot be upgraded, return a structure containing only the problems.
  """

  #raise NotImplementedError()
  return True

