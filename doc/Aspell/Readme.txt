This folder contains spellchecker dictionaries for Aspell.
One for each language and a generic template file that is copied to each language.

TortoiseGit.tmpl.pws:
  Generic dictionary of terms & Keywords used in the TortoiseGit manual. 
  This file is language independent and is copied to each target language 
  (named temp.pws), when the translation takes place.

en.pws:
  Dictionaries of English terms.

To execute the spell checker on the documentation:
 * Install Python
 * Install Aspell and the English Aspell dictionary.
 * python3 check.py

Spell checking has only been tested on *nix-bases systems recently.

The output of the spellchecking process is written to "*.log".
