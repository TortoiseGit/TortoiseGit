#!/bin/bash

if [[ "$CI" != 'true' ]]; then
	echo "no CI detected"
	exit 1
fi

function section_start() {
	local section_title="${1}"
	local section_description="${2:-$section_title}"
	echo -e "section_start:`date +%s`:${section_title}[collapsed=false]\r\e[0K\e[1;96m${section_description}\e[0m"
}
function section_end() {
	local section_title="${1}"
	echo -e "section_end:`date +%s`:${section_title}\r\e[0K"
}

rm -f src/Resources/TGitHelpMapping.ini src/Resources/TGitMergeHelpMapping.ini src/TortoiseGitSetup/HTMLHelpfiles.wxi

err=0
pushd "$CI_PROJECT_DIR/doc" > /dev/null

section_start "build_doc" "Building the documentation"
(set -x; python3 build.py)
result=$?
section_end "build_doc"
if [[ $result -ne 0 ]]; then
	echo -e "⚠️ \e[1;31mBuilding documentation failed, see above for details! ⚠️\e[0m"
	err=1
fi
section_start "spellcheck_doc" "Spell checking the documentation"
pushd Aspell > /dev/null
(set -x; python3 check_doc.py)
result=$?
popd > /dev/null
section_end "spellcheck_doc"
if [[ $result -eq 2 ]]; then
	echo -e "⚠️ \e[1;31mFound typos in documentation: ⚠️\e[0m";
	err=1
	cat Aspell/*.log;
elif [[ $result -ne 0 ]]; then
	echo -e "⚠️ \e[1;31mSpellchecking the documentation failed, see above for details! ⚠️\e[0m"
	err=1
else
	echo -e "✔️ \e[32;1mNo spell errors detected in documentation. ✔️\e[0m"
fi

section_start "idd_inconsistencies" "Checking for unused or missing IDDs"
(set -x; ./CheckIDD.sh)
result=$?
section_end "idd_inconsistencies"
if [[ $result -ne 0 ]]; then
	echo -e "⚠️ \e[1;31mFound inconsistencies, see above ⚠️\e[0m";
	err=1
else
	echo -e "✔️ \e[32;1mNo inconsistencies found. ✔️\e[0m"
fi
popd > /dev/null

section_start "helpmapping_inconsistencies" "Checking for (uncommitted) inconsistencies in the help mapping files"
git diff --no-prefix --color -- src/Resources/TGitHelpMapping.ini src/Resources/TGitMergeHelpMapping.ini src/TortoiseGitSetup/HTMLHelpfiles.wxi
result=$?
git diff-index --no-prefix --color --quiet HEAD -- src/Resources/TGitHelpMapping.ini src/Resources/TGitMergeHelpMapping.ini src/TortoiseGitSetup/HTMLHelpfiles.wxi
result=$(($result + $?))
section_end "helpmapping_inconsistencies"
if [[ $result -ne 0 ]]; then
	echo -e "⚠️ \e[1;31mFound inconsistencies, see above ⚠️\e[0m";
	err=1
else
	echo -e "✔️ \e[32;1mNo inconsistencies found. ✔️\e[0m"
fi

exit $err
