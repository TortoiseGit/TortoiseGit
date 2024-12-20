#!/bin/bash
# Use a Python script to cross check IDD_ values in the resource file with
# HIDD_ values in the docs to ensure that every dialog has a link to the help.
tmpfile=`mktemp`
dirname=$(dirname -- "$0")
pushd "$dirname" > /dev/null
python3 scripts/chkidd.py source/en/TortoiseGit ../src/Resources/TortoiseProcENG.rc scripts/TortoiseProcENG.ignore > "$tmpfile"
python3 scripts/chkidd.py source/en/TortoiseMerge ../src/Resources/TortoiseMergeENG.rc scripts/TortoiseMergeENG.ignore >> "$tmpfile"
popd > /dev/null
retcode=0
if grep -q -v '^#' "$tmpfile"; then
	cat "$tmpfile"
	retcode=1
fi
rm -- "$tmpfile"
exit $retcode
