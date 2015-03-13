/code/git/tae/tae -d -S safplusTest.xml

Using this example

Step:

1. Compile the "c" library in the parent directory
$ (cd ..; make)

2. Edit model.cfg
model.cfg
Change the <project> tag to an existing project within your tae report server.

ex:
<project value="myproject" />

Change the <project_dir> to this directory's parent.
ex:
<project_dir  value="/home/user/openclovis-tae-X.Y/c" />


3. Edit env.cfg
Change the report URL to point to the tae report server import directory

ex:
<report_url value="scp://user:password@localhost://home/user/openclovis-tae-X.Y/taereport/import" />

4. Build and run!
$ make
$ make test
