#!/bin/bash

APIREFDIR=$WORKDIR/svn/doc/apirefs

# got to the working dir's apirefs directory
cd $APIREFDIR

# re-build the html pages with search engine
# copy the html page tree to the server
cp Doxyfile x && \
	echo SEARCHENGINE=YES >> x && \
	make DOXYFILE=x all && \
	rm -fr x && \
	rsync -avDH --delete html/ root@192.168.0.94:/var/www/intranet/apirefs
