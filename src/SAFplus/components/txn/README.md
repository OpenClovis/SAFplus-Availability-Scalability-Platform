## Transaction Manager

The OpenClovis Transaction Manager (TM) is an infrastructure service for providing the 2-phase commit transaction semantics. Along with this it provides two libraries the client library and the agent library. The client library is used to submit the transaction jobs and also the information about all the participating components. Any component that wishes to be a part of transactions can link with the agent library and act as a resource manager.

- Features

Supports re-startable transactions. For instance, if the Transaction Manager dies, all the pending transactions will be restarted when it is coming up again. 

- How it works

It tracks the participants and provides ACID semantics to ensure that all the participants are updated properly, or will rollback to previous state assuring data integrity despite component failures.
