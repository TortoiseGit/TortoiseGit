#!/bin/bash
# parameters
# $1 = path to spellcheck executable
# $2 = language
# $3 = name of the file to check
# $4 = name of the logfile
echo --- $3 > $4
xsltproc --nonet ./Aspell/removetags.xsl "$3" | "$1" --mode=sgml --encoding=utf-8 --add-extra-dicts="./Aspell/$2.pws" --add-extra-dicts=./Aspell/Temp.pws --lang="$2" list check >> "$4"
