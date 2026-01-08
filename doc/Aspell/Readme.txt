Spell checking has only been tested on *nix-bases systems.

Install Aspell and all the Aspell dictionaries you need.

TODO: Activate spellchecking by setting two properties in doc/doc.build.user.

TODO: <property name="spellcheck" value="true"/> 
TODO: <property name="path.spellcheck" value="C:\Path\to\your\Aspell.exe"/>

This folder contains spellchecker dictionaries for Aspell. 
One for each language and a generic template file that is copied to each language.

TortoiseGit.tmpl.pws:
  Generic dictionary of terms & Keywords used in the TortoiseGit manual. 
  This file is language independent and is copied to each target language 
  (named temp.pws), when the translation takes place.

de.pws, en.pws, ...:
  Dictionaries of terms for each language.

The process is still kludgy:
- Aspell doesn't work for Asian languages (yet). No fix available.
- It will fail if <NN>.pws doesn't exist yet. 
  Create a text file <NN>.pws that just contains the line:
  "personal_ws-1.1 <NN> 0" where <NN> is the country code.
  Look in de.pws or en.pws for examples. 
  Add your terms after this line.

The output of the spellchecking process is written to "spellcheck_<NN>.log".
