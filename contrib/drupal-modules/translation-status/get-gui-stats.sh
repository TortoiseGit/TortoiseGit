#!/bin/sh
#
# Calculate po translation statistics of all po files inside $CATALOGS_DIR
# and write the result to standard output
#
# Copyright (C) 2004-2008 the TortoiseSVN team
# This file is distributed under the same license as TortoiseSVN
#
# $Author: luebbe $
# $Date: 2008-06-23 17:52:54 +0800 (Mon, 23 Jun 2008) $
# $Rev: 13326 $
#
# Author: Lübbe Onken 2004-2008
#

HOME_DIR=/var/svnwc/l10n

MSGFMT=msgfmt
MSGMERGE=msgmerge
SVNVERSION=svnversion
CATALOGS_DIR="$HOME_DIR/$1/gui"
TEMP_DIR="$CATALOGS_DIR/tmp"

WCREV=`$SVNVERSION $CATALOGS_DIR | sed -e 's/[MS]//g'`
UPDATE=`date`

rm -rf $TEMP_DIR
mkdir $TEMP_DIR

echo '<?php'
echo '$TortoiseGUI=array('

TO=0
x=`$MSGFMT --verbose -o /dev/null ${CATALOGS_DIR}/Tortoise.pot 2>&1 | grep 'translated messages' | \
   sed -e 's/[,\.]//g' \
       -e 's/\([0-9]\+\) translated messages\?/TR=\1/' \
       -e 's/\([0-9]\+\) untranslated messages\?/TO=\1/'`
eval $x

APP=Tortoise
for i in ${CATALOGS_DIR}/*.po ; do
   ER=0 AK=0 TR=0 FZ=0 UT=0
	catname=`basename $i .po`
	country=`basename $i .po | sed -e 's/Tortoise_//'`
	tempfile="$TEMP_DIR/$catname.po"
	filedate=`stat -c "%y" ${CATALOGS_DIR}/$catname.po`

	x=`cat ${CATALOGS_DIR}/$catname.po | grep 'PO-Revision-Date:' | \
           sed -e 's/"PO-Revision-Date: //g' \
               -e 's/[0-9]\{2\}:[0-9]\{2\}.*//g' \
               -e 's/\([0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}\)/PRD=\1/'`
	eval $x

	cp $i $tempfile 
	$MSGMERGE --no-wrap --quiet --no-fuzzy-matching -s $i  ${CATALOGS_DIR}/${APP}.pot -o $tempfile 2>/dev/null
	
	x=`$MSGFMT -c -o /dev/null $tempfile 2>&1 | grep 'fatal error' | \
		sed -e 's/[^0-9]//g' \
		    -e 's/\([0-9]\+\)\?/ER=\1/'`
	eval $x

	if test $ER -eq 0 ; then

		x=`$MSGFMT --verbose -o /dev/null $tempfile 2>&1 | grep 'translated messages' | \
			sed -e 's/[,\.]//g' \
				-e 's/\([0-9]\+\) translated messages\?/TR=\1/' \
        	 		-e 's/\([0-9]\+\) fuzzy translations\?/FZ=\1/' \
				-e 's/\([0-9]\+\) untranslated messages\?/UT=\1/'`
  		eval $x

		x=`$MSGFMT --check-accelerators -o /dev/null $tempfile 2>&1 | grep 'fatal error' | \
			sed -e 's/[^0-9]//g' \
		      	       -e 's/\([0-9]\+\)\?/AK=\1/'`
	   	eval $x
	fi
	if test $UT -eq 0 ; then
		UT=$((TO-TR-FZ))
	fi
   echo "\"$country\" => array($ER, $TO, $TR, $FZ, $UT, $AK, \"$catname\", \"$PRD\"),"
done

# Separate line for the .pot file
filedate=`stat -c "%Y" ${CATALOGS_DIR}/${APP}.pot`
echo "\"zzz\" => array(-1, $TO, 0, 0, $TO, 0, \"${APP}.pot\",\"$filedate\")"

echo ');'
echo '$tsvn_var = array('
echo "\"wcrev\" => \"$WCREV\","
echo "\"update\" => \"$UPDATE\","
echo ');'
echo '?>'

rm -rf $TEMP_DIR

