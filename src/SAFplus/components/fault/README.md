## Fault Manager

The OpenClovis Fault Manager (FM) infrastructure provides a hierarchical scheme for managing faults in a system and initiating actions as configured during the design time. It can handle various user-defined run-time faults, including hardware and software faults and can prioritize faults to ensure that the critical faults are addressed before the normal or the low-priority faults.

The alarms are notified by the Fault Manager client library to the Fault Manager server located on the same node. The actions to be taken on receiving a fault are controlled by the FM policy associated with the faults. 

For APIs reference guides, please access doc/html/apirefs/index.htm, then click Fault Manager
