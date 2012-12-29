@echo off

..\tools\convert.exe -resize 32x32 @ribbonimagelist.txt +append ribbonlarge.png
..\tools\convert.exe -resize 16x16 @ribbonimagelist.txt +append ribbonsmall.png

if exist ..\*.aps del ..\*.aps

pause
