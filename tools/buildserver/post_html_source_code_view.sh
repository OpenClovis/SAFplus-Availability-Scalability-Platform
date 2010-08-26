#!/bin/bash

CONFFILE=$PWD/globalrc       # globalrc config file is expected in local dir
ASPDIR=$WORKDIR/svn/pkg/src/ASP

cd $ASPDIR

echo "Built from svn tree $SVN_PATH build number $BUILD_NUMBER" \
    > $ASPDIR/header.html
htags --gtagsconf $CONFFILE && \
    rsync -avDH --delete HTML/ root@192.168.0.94:/var/www/intranet/src/ASP
