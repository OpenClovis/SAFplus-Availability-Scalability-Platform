# **Safplus_console user manual**

## **Introduction**

safplus_console is one of the tools of safplus 6.0. It’s located at the bin/ directory of each node image. It consists of many commands helping you query information about the entities and interact with them. Additionally, it provides many commands to retrieve information and status of safplus core components like gms, ckpt, msg, event, name, log. You can also execute some APIs of those components at runtime.

After running safplus_console, type ‘help’ or ? at the command line, you’ll see the list of commands in this context. For example, you’ll see commands below at the first context:

cli[Test]-> ?

setc - set context to node

bye,quit,exit - quit the cli

end - quit the current context and go to the previous one

help - lists commands in current mode

? - lists commands in current mode

list - list nodes to which setc can be done in current mode

history - display the last 100 commands executed

timeoutset - set the timeout for the debug session

timeoutget - get the timeout of the debug session

sleep - sleep for the specified time

errno - return code of the last executed call

status - sucess/failure status of the last executed call

And when type ‘list’, you’ll see all the possible arguments for the setc command in this context. For example, 

cli[Test]-> list

Slot	Node

1	SCNodeI0

In this context, you can setc 1. After that, type ‘list’ you’ll see the following context:

amf

cpm

ioc

ckptServer_SCNodeI0

gmsServer_SCNodeI0

nameServer_SCNodeI0

logServer_SCNodeI0

eventServer_SCNodeI0

msgServer_SCNodeI0

Let’s discover how to use some commands needing more explanations in the following section. For other commands not listed here, please see help on the console.

### amf or cpm context: type help or ?

1. timersShow: shows the started timers

           Syntax: timersShow

2. rmdStatsShow: shows the RMD stats

           Syntax: rmdStatsShow
   
3. start: starts an entity running and assigns work, regardless of its current state. This command will perform some operations so that the admin state of the specified entity turns into **UNLOCKED.** Valid entities are node, su, sg, si.

           Syntax: start <entity name>
   
4. stop: stops an entity regardless of its current state. This command will perform some operations so that the admin state of the specified entity turns into **LOCKED_INSTANTIATION.** Valid entities are node, su, sg.

           Syntax: stop <entity name>
   
5. idle: Puts an entity in the idle state (running but no assigned work), regardless of its current state. This command will perform some operations so that the admin state of the specified entity turns into **LOCKED_ASSIGNMENT.** Valid entities are node, su, sg, si.

           Syntax: idle <entity name>
   
6. repair: repairs a faulty entity, which means the entity’s operational state or the operational state of the container’s entity is DISABLED . This command will perform some operations so that the admin state of the specified entity or container’s entity turns into **UNLOCKED.** Valid entities are node, su.

           Syntax: repair <entity name>
   
7. repair all: looks through every faulty entity and repairs it. Faulty entity means the entity's operational state or the operational state of the container’s entities are DISABLED . This command will perform some operations so that the admin state of the faulty entities or container’s entities turns into **UNLOCKED.**

           Syntax: repair all
   
8. amsLockAssignment: performs lock-assigment operation on an SG,SU,NODE or SI. This command is successful only if the current administrative state of the specified entity is **UNLOCKED** or **LOCKED_INSTANTIATION**

           Syntax: amsLockAssignment <sg, su, node or si> <entity name>
   
           Example: amsLockAssignment node SCNodeI0
   
9. amsLockInstantiation: performs lock-instantiation operation on an SG,SU,NODE. This command is successful only if the current administrative state of the specified entity is **LOCKED_ASSIGNMENT**  
   Syntax: amsLockInstantiation <sg, su or node> <entity name>
   
           Example: amsLockInstantiation node SCNodeI1  
   
10. amsUnlock: performs unlock operation on an SG,SU,NODE or SI. This command is successful only if the current administrative state of the specified entity is **LOCKED_ASSIGNMENT**  

           Syntax: amsUnlock <sg, su, node or si> <entity name>  
    
           Example: amsUnlock su ScSUI0  
    
11. amsShutdown: same as amsLockAssignment (see amsLockAssignment command)  

12. amsRestart: Admin API for restarting an entity

           Syntax: amsRestart <node, su or comp> <entity name>

           Example: amsRestart su ScSUI0

13. amsRepaired: Admin API for marking an entity as repaired (same as repair command)

           Syntax: amsRepaired <node, su> <entity name>

           Example: amsRepair su ScSUI0

14. amsFaultReport: admin API for reporting a fault on a component/node. Assuming that a component or node is not faulty and it’s running normally. But if you want it do a failover, you should run this command

            Syntax: amsFaultReport <node name or component name> <recommended_recovery>

       Parameters:

       * recommended_recovery: is one of values:  
         *  0: None  
         *  1: NO_RECOMMENDATION             
         *  2: COMP_RESTART                  
         *  3: COMP_FAILOVER                 
         *  4: NODE_SWITCHOVER               
         *  5: NODE_FAILOVER                 
         *  6: NODE_FAILFAST                 
         *  7: CLUSTER_RESET                 
         *  8: APP_RESTART                 

       Example:

            amsFaultReport compI0 2          


15. compReport: report an error on a component. Assuming that a component is not faulty and it’s running normally. But if you want it do a failover, you should run this command  

           Syntax: compReport <comp name> <report time> <recommended_recovery>  
    
       Parameters:  
       * recommended_recovery: is one of values:  
         *  0: None  
         *  1: NO_RECOMMENDATION             
         *  2: COMP_RESTART                  
         *  3: COMP_FAILOVER                 
         *  4: NODE_SWITCHOVER               
         *  5: NODE_FAILOVER                 
         *  6: NODE_FAILFAST                 
         *  7: CLUSTER_RESET                 
         *  8: APP_RESTART    

       Example: 

            compReport compI0 3

16. amsMgmt: creates, deletes or migrates an entity in the current cluster. This command also changes the attributes of an entity.

            Syntax: amsMgmt create|delete|migrate sg|si|csi|node|su|comp entity-name

            Or amsMgmt set sg|si|csi|node|su|comp attribute [value1 ]+ entity-name

       Parameters:

       * entity-name: name of the entity to be done  
         * attribute: name of the attribute of the entity. One or many attributes can be set at the one call:  
           * With Service Group:  
             *  si_list: si name to be added to the list  
             *  su_list: su name to be added to the list  
             *  redundancy_model:  
               *  no_redundancy  
               *  twon: two- N redundancy  
               *  mplusn: M plus N redundancy  
               *  loading_strategy:  
               *  least_si_per_su  
               *  least_su_assigned  
               *  least_load_per_su  
               *  by_si_reference  
             * auto_repair:
               *  true  
               *  false  
             * instantiate_duration  
               *  num_pref_active_sus  
               *  num_pref_standby_sus  
             * num_pref_inservice_sus  
             * num_pref_assigned_sus  
             * max_active_sis_per_su  
             * max_standby_sis_per_su  
             * comp_restart_duration  
             * comp_restart_count_max  
             * su_restart_duration  
             * su_restart_count_max  
           * With Service Instance:  
             * rank  
             * num_standby_assignments  
             * csi_list: csi name to be added to the list  
           * With Component Service Instance  
             * name_value_pair: e.g. n v  
           * With node:  
             * su_list: su name to be added to the list  
             * is_restartable:  
               * true  
               * false  
             * auto_repair:  
               * true  
               * false  
             * su_failover_duration  
             * su_failover_count_max  
           * With Service Unit:  
             * comp_list: comp name to be added to the list  
             * rank  
             * is_restartable: true or false  
           * With Component:  
             * supported_csi_types: csi type to be added to the list  
             * capability_model:  
               * x_and_y  
               * x_or_y  
               * one_or_x  
               * one_or_one  
               * x_active  
               * one_active  
             * is_restartable: true or false  
             * instantiate_level  
             * num_max_instantiate  
             * num_max_instantiate_with_delay  
             * num_max_active_csis  
             * num_max_standby_csis  
             * timeouts  
             * instantiate_delay  
             * recovery_on_timeout:  
               * no_rec: no recommendation  
               * comp_failover  
               * comp_restart  
               * node_failover  
               * node_switchover  
               * node_failfast  
             * Instantiate_command: e.g. cmd -foo -bar -foobar barz

       Examples:

            amsMgmt create comp NewCompI1

            amsMgmt set comp is_restartable true NewCompI1

            amsMgmt set comp capability_model x_and_y recovery_on_timeout comp_failover NewCompI1

            amsMgmt delete comp NewCompI1

### ioc context: 

Entering this context, you can get/add/delete multicast peers list. This context is only available when the current transport used is UDP.

### Other contexts: ckptServer, gmsServer, nameServer, logServer, eventServer, msgServer

1. ckptServer: you can enter this context to do some operations with checkpoints currently in your cluster: view checkpoint information and status, create/update/delete checkpoints and their sections, read/write data to sections, update some attributes of checkpoints. Follow the help for each command in this context to gain your needs.

2. gmsServer: you can enter this context to do some operations with groups currently in your cluster: view group information and status, member list, leader election…Follow the help for each command in this context to gain your needs.

3. logServer: you can enter this context to do some operations with logging: get stream list, open and write data to a stream…Follow the help for each command in this context to gain your needs.

4. msgServer: you can enter this context to view message queue information. Follow the help for each command in this context to gain your needs.

5. eventServer: you can enter this context to do some operations with an event: open event channel, subscribe, publish event, … Follow the help for each command in this context to gain your needs.
