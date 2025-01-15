# **Safplus_info user manual**

## **Introduction**

safplus_info is one of the tools of safplus 6.0. It’s located at the bin/ directory of each node image. It consists of many commands helping you query information about the entities and interact with them.

After running safplus_info, type ‘help’ or ? at the command line, you’ll see all commands:

[safplus_info@SCNodeI0]==> help

Available commands (type help <topic> for more info):

=====================================================

EOF  cluster 	forest  lock_a   nodes  role 	sg 	status  tree  

asp  components  help	lock_i   quit   safplus  sgs	su  	unlock

bye  exit    	host	raw	setup	shell  sus 	uptime

Undocumented commands:

======================

version

Type help <command> you’ll see the brief help for this command. For example, *help cluster*

 Let’s discover how to use the commands in the following section.

1. asp: shows all safplus components running in this node. Note that, safplus components are the safplus core components such as gms, msg, checkpoint… not user’s components. If you want to see the user’s components, run components (see components command)

           Syntax: asp

2. bye: exits this program

           Syntax: bye
           
3. cluster: lists all the nodes running in this cluster and their role (NODE-TYPE), high availability state (HA-STATE) and node address (NODE-ADDR).

   Role: can be worker (payload) or controller
   
   HA state: can be active, standby or no state (-) (in case the role of node is worker)
   
           Syntax: cluster
   
4. Components: shows all user’s components running in this node. If you want to see the safplus core components, run safplus or asp (see asp or safplus command)

           Syntax: components

5. exit: same as bye

6. forest: shows the HA hierarchy for each SG in the cluster (call 'tree' for each SG). With this command, you’ll see the SG, Node, SI, SU, Comp, CSI and their administrative state (Locked/Unlocked) as well as the HA state of SI and CSI.

           Syntax: forest

7. help: shows help for a command.

           Syntax: help <command_name>

8. host: shows various information (OS/computer/blade) including CPU, Memory, Disk, Network… in this node.

            Syntax: host

9. lock_a: performs lock-assigment operation on an SG,SU,NODE or SI. This command is successful only if the current administrative state of the specified entity is **UNLOCKED** or **LOCKED_INSTANTIATION**

            Syntax: lock_a <sg, su, node or si> <entity name>
   
            Example: lock_a node SCNodeI0

10. lock_i: performs lock-instantiation operation on an SG,SU,NODE. This command is successful only if the current administrative state of the specified entity is **LOCKED_ASSIGNMENT**

            Syntax: lock_i <sg, su or node> <entity name>
    
            Example: lock_i node SCNodeI1
    
11. nodes: shows list of nodes running in the cluster and their states

            Syntax: nodes
    
12. quit: same as bye or exit

13. raw: runs a batch of safplus_console commands separated by a semicolon (;)  and print status and output on screen.

            Syntax: raw <cmd> [; <cmd> [; <cmd> ...]]

            Example: raw setc master; setc amf; amsEntityPrint node SCNodeI0 → this will run the amsEntittyPrint command of safplus_console and the output will be shown on the screen

14. role: show the HA state and node type of this node. HA state can be active/standby if the node type is controller; if node type is worker or payload, there is no HA state

            Syntax: role
            
15. safplus: same as asp command (see asp command)

16. setup: shows various information on this SAFplus installation (does not need SAFplus running) including installation status, node instance name, node class, node type, default node address, node address assignment, tipc netid, tipc address, cluster interface(s), multicast ip, gms multicast port. If the current transport is UDP, there is no information of tipc

            Syntax: setup
            
17. sg: shows the properties and status of the specified service group as the argument. If you don’t remember a name of SG, you can run sgs command (see sgs command)

            Syntax: sg <sg name>

18. sgs: shows the list of service group defined in the cluster

           Syntax: sgs

19. shell: executes the command as the passed arguments and print the result

            Syntax: shell <command>

            Example: shell pwd

20. status: shows SAFplus running status and some basic run-time information of this node deployed in this sandbox

            Syntax: status

21. su: shows the properties and status of the specified service unit as the argument. If you don’t remember a name of the SU, you can run sus command (see sus command)

            Syntax: su <su name>

22. sus: shows list of service units

            Syntax: sus [<node name | all>]
    
    Arguments:  
* If no arguments passed, this shows the list of SU in this node  
* If node name is passed, this shows the list of SU in the node  
* If ‘all’ is passed, this shows all SUs defined in the cluster   

23. tree: prints the HA hierarchy for the SG as the passed argument in the cluster (see forest command also)

            Syntax: tree <sg name>

24. unlock: performs unlock operation on an SG,SU,NODE or SI. This command is successful only if the current administrative state of the specified entity is **LOCKED_ASSIGNMENT**

            Syntax: unlock <sg, su, node or si> <entity name>
    
            Example: unlock su ScSUI0
    
25. uptime: shows the node uptime, which means the duration of time since the node started

            Syntax: uptime
