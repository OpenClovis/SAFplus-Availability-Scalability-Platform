#!/bin/sh 

SRCDIR=$1
DESTDIR=$2

cd ${SRCDIR}
# Features
find features -type f | while read _feature ; do
if [[ ${_feature} =~ (.*\.jar$) ]] ; then
  install -dm755 ${DESTDIR}/${_feature%*.jar}
  cd ${DESTDIR}/${_feature/.jar}
  jar xf ${SRCDIR}/${_feature} || return 1
else
  install -Dm644 ${SRCDIR}/${_feature} ${DESTDIR}/${_feature}
fi
done

# Plugins
find plugins -type f | while read _plugin ; do
    install -Dm644 ${_plugin} ${DESTDIR}/${_plugin}
done