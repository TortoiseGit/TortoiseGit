Spell checking has only been tested on *nix-bases systems recently.

Install Aspell and all the English Aspell dictionaries.

This folder contains spellchecker dictionaries for Aspell.
One for each language and a generic template file that is copied to each language.

TortoiseGit.tmpl.pws:
  Generic dictionary of terms & Keywords used in the TortoiseGit manual. 
  This file is language independent and is copied to each target language 
  (named temp.pws), when the translation takes place.

en.pws:
  Dictionaries of English terms.

The output of the spellchecking process is written to "spellcheck_<NN>.log".
