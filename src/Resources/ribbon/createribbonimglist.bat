@echo off

..\tools\convert.exe -resize 32x32 @ribbonimagelist.txt +append ribbonlarge.png
..\tools\convert.exe -resize 16x16 @ribbonimagelist.txt +append ribbonsmall.png
..\tools\convert.exe -resize 48x48 @ribbonimagelist.txt +append ribbonlargehdpi.png
..\tools\convert.exe -resize 24x24 @ribbonimagelist.txt +append ribbonsmallhdpi.png

if exist ..\*.aps del ..\*.aps

pause
