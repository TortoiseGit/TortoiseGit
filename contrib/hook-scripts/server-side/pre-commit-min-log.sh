#!/bin/sh
REPOS="$1"
TXN="$2"
SVNLOOK=/usr/bin/svnlook

# check that logmessage contains at least 10 alphanumeric characters
LOGMSG=`$SVNLOOK log -t "$TXN" "$REPOS" | grep "[a-zA-Z0-9]" | wc -c`
if [ "$LOGMSG" -lt 10 ];
then
  echo -e "\nEmpty log message not allowed. Commit aborted!" 1>&2
  exit 1
fi
