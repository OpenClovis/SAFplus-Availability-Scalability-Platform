## Database Abstraction Layer (DBAL)

The OpenClovis Database Abstraction Layer (DBAL) provides a standard interface for any OpenClovis SAFplus Platform infrastructure component or application to interface with databases.

### Features

- DBAL currently supports:
   - the SQLite database
   - the GNU Database Manager (GDBM).
   - the Oracle Berkeley DB 

### How it Works

The primary user of this interface is the COR component which can write or read its object repository to and from a database for either persistent storage or offline processing. DBAL is also a standalone library with no dependencies on any other OpenClovis SAFplus Platform components.

For information about selecting a database engine see https://help.openclovis.com/index.php/Doc:latest/sdkguide/compconfig 

For APIs reference guides, please access doc/html/apirefs/index.htm, then click Database Abstraction Layer (DBAL)
