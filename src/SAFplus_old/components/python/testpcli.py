import pdb

from nodeExpect import *

myPassword = "clovis"

def Test():

  node = Node("anode","/root/asp","127.0.0.1","root",myPassword)
  print node.run("/bin/ls")


