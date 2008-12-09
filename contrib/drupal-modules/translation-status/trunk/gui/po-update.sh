#!/bin/sh
#
# Usage:
# ./po-update.sh
#   - to update all locales
# ./po-update.sh LL
#   - to update one the LL locale

set -e

MSGMERGE=${MSGMERGE:-msgmerge}

tsvn_base=
for i in . .. ../..; do
  if [ -d "$i/tortoisesvn/Languages" ]; then
    tsvn_base="$i"
    break
  fi
done
if [ -z "$tsvn_base" ]; then
  echo "E: You must run po-update.sh from within a TortoiseSVN source tree." >&2
  exit 1
fi

update_po()
{
  (cd $tsvn_base/tortoisesvn/Languages &&
  for i in $1.po; do
    echo "Updating $i..."
    msgmerge --no-wrap -o tmp.po $i Tortoise.pot
    mv -f tmp.po $i
  done )
}

if [ $# -eq 0 ]; then
  update_po \*
else
  langs=
  while [ $# -ge 1 ]; do
    case $1 in
      pot) ;;
      *)
      if [ -e $tsvn_base/tortoisesvn/Languages/Tortoise_$1.po ]; then
        langs="$langs Tortoise_$1"
      else
        echo "E: No such .po file 'Tortoise_$1.po'" >&2
        exit 1
      fi
    esac
    shift
  done
  for lang in $langs; do
    update_po $lang
  done
fi

