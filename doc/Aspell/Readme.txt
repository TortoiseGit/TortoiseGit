This folder contains spell checker script and dictionaries for Aspell.

Structure
---------

TortoiseGit.tmpl.pws:
  Generic dictionary of terms & Keywords used in the TortoiseGit manual.

en.pws:
  Additional dictionary of English terms.

Tools needed
------------

* Python3
* xsltproc
* Aspell and the English Aspell dictionary

Note, spell checking has only been used on *nix-bases systems recently (in the CI pipeline),
but can also be used with a MSYS2 environment (pacman -S libxslt aspell aspell-en).

Paths can be configured using the two properties in doc/doc.build.user:

<property name="path.bin" value="/usr/bin"/>
<property name="path.spellcheck" value="aspell"/>

How to run?
-----------

Execute: python3 check_doc.py

The output of the spell checking process is written to "*.log".
