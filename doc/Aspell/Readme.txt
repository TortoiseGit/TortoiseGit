This folder contains spell checker script and dictionaries for Aspell.

TortoiseGit.tmpl.pws:
  Generic dictionary of terms & Keywords used in the TortoiseGit manual. 

en.pws:
  Additional dictionary of English terms.

To execute the spell checker on the documentation:
 * Install Python
 * Install Aspell and the English Aspell dictionary.
 * python3 check_doc.py

Spell checking has only been tested on *nix-bases systems recently.

The output of the spellchecking process is written to "*.log".
