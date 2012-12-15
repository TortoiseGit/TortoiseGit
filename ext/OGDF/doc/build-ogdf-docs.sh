#!/bin/bash 

sourcepath="$0"

# folder wich contains the script
folder="${sourcepath%/*}"
 
cd "$folder"
rm -r ./html
doxygen "ogdf-doxygen.cfg"
