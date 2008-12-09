MakeUTF8.c
==========
Description:
This program checks text files for the presence of a byte-order-mark (BOM)
and for a UTF-8 encoding indicator in the XML version tag. You can also
opt to add either or both of these features.

Use:
MakeUTF8 [ -b ] [ -x ] file [ file ... ]
Wildcard filenames are supported. Subdirectory recursion is not at present.
-b option adds/corrects BOM in file if not already present.
-x option adds/corrects XML tag if not already present.
With no options, the current stateis reported but nothing is changed.

Example:
MakeUTF8 -b *.xml tsvn_dug\*.xml
Fixes BOMs (but not XML tags) in all .xml files in the current directory,
and in the tsvn_dug subdirectory.
