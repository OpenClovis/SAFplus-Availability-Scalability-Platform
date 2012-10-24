#!/usr/bin/env bash
CC="$1"
if [ ! -x ${CC} ];
  then
    CC="gcc"
  fi
CC="${CC}"
( ${CC} -x c - -o check_sctp >/dev/null 2>&1 && ./check_sctp || echo "failed") <<EOF
#include <unistd.h>
#include <netinet/sctp.h>
int main() {
    return close(socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP));
}
EOF
rm -f ./check_sctp