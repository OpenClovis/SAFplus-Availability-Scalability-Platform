"""@namespace examples.xml

This shows a xml-rpc client protocol for program-to-program
communication.  For example, your EMS could use this protocol (extended,
of course) to control the cluster.

This is the client.  The server should be started first.

"""
import pdb
import xmlrpclib

XMLRPC_PORT = 8001


def main(machine,port):
    s = xmlrpclib.ServerProxy('http://%s:%d' % (machine,port))

    # Print list of available methods
    print "RPC METHODS:\n ", s.system.listMethods(), "\n\n"

    print "NODES, SUs, and Components:\n ", s.GetCluster(), "\n\n"

    nodes = s.GetNodes()
    print "NODE LIST:\n", nodes, "\n\n"

    print "SERVICE UNIT LIST:\n", s.GetServiceUnits(), "\n\n"

    print "SERVICE GROUP LIST:\n", s.GetServiceGroups(), "\n\n"

    # Add an application bundle to the repository
    # The expectation is that this file would have already been uploaded via some standard
    # file transfer protocol such as ftp or scp

    print "ADDING APPLICATION BUNDLE\n"
    try:
      # ***Of course this path needs to be changed to a valid application bundle on your system***
      s.NewApp("/code/vipapp1.4.0.0.tgz")
    except xmlrpclib.Fault,e:
      print "File not found.  You must call NewApp() with a valid application bundle file that you have copied (ftp, scp) to the server node!"
      # pdb.set_trace()
      return 

    # Show available applications
    print "APPLICATION LIST:\n", s.GetApplications(), "\n\n"

    print "DEPLOYING APP (may take some time):\n"
    deployed = s.Deploy("virtualIp","1.4.0.0", [nodes[0]])
    print deployed, "\n\n"

    # Add another application bundle to the repository
    # ***Of course this path needs to be changed to a valid application bundle on your system***
    print "ADDING NEW APPLICATION BUNDLE VERSION\n"
    try:
      # ***Of course this path needs to be changed to a valid application bundle on your system***
      s.NewApp("/code/vipapp1.4.0.1.tgz")
    except xmlrpclib.Fault,e:
      print "File not found.  You must call NewApp() with a valid application bundle file that you have copied (ftp, scp) to the server node!"
      # pdb.set_trace()
      return 

    print "UPGRADING APP (may take some time):\n", s.Upgrade(deployed[0][0],"1.4.0.1"), "\n\n"


if __name__=="__main__":
  import sys
  print "Usage: %s [machine] [port]\n" % sys.argv[0]
  if len(sys.argv) >= 2:
    machine = sys.argv[1]
  else: machine = "localhost"

  if len(sys.argv) >= 3:
    port = int(sys.argv[2])
  else: port = XMLRPC_PORT

  main(machine,port)
