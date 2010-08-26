#!/bin/bash

HTML_DIR=$WORKDIR/svn/doc/html

rsync -avDH --delete $HTML_DIR/ root@192.168.0.94:/var/www/intranet/userdocs

