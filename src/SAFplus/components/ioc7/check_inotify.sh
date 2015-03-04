#!/usr/bin/env bash
CC="$1"
if [ ! -x ${CC} ];
  then
    CC="gcc"
  fi
CC="${CC}"
( ${CC} -x c - -o check_inotify >/dev/null 2>&1 && ./check_inotify || echo "failed") <<EOF
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
int main() {
    return close(inotify_init());
}
EOF
rm -f ./check_inotify