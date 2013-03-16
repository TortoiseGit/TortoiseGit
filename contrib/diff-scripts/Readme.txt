This folder contains scripts which can be used to start other programs
to diff non-text files.

The scripts are named after the file extension they're able to diff.
E.g. the script to diff word files is called 'diff-doc.vbs', the script
to merge three word files is called 'merge-doc.vbs'.

The scripts must be implemented so that they take the following parameters:
diff-ext %base %mine
merge-ext %merged %theirs %mine %base

The scripts can then decide themselves if they need all the params or if
they have to add additional params to the application they call.
