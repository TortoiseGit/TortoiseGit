@echo off
:: Use a Python script to cross check IDD_ values in the resource file with
:: HIDD_ values in the docs to ensure that every dialog has a link to the help.
python scripts\chkidd.py source\en\TortoiseGit ..\src\Resources\TortoiseProcENG.rc scripts\TortoiseProcENG.ignore > IDD_Errors.txt
python scripts\chkidd.py source\en\TortoiseMerge ..\src\Resources\TortoiseMergeENG.rc scripts\TortoiseMergeENG.ignore >> IDD_Errors.txt
