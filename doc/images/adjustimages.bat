@echo off & setlocal enableextensions enabledelayedexpansion
:: ============================================================ 
:: This script compresses all .png images.
::
:: By default all images in all languages will be handled.
:: You can specify a group of files to check using the first
:: parameter, e.g.
::     adjustimages en\Rev*.png
::
:: ============================================================ 
::
::
if '%1' == '' goto :DoDefault
for %%? in (%1) do Call :ProcAdjustFile %%?
endlocal & goto:EOF
::
:DoDefault
for %%? in (en\*.png) do Call :ProcAdjustFile %%?
endlocal & goto :EOF
::===============================================================
:ProcAdjustFile FileName
:: Compress file
echo Compressing %1
type %1 | pngquant.exe 256 > 8bit.tmp
del %1 > nul
move 8bit.tmp %1 >nul
optipng.exe -o7 -quiet %1
pngout.exe %1 /y /d0 /s0 /mincodes0 >nul
DeflOpt.exe %1 >nul
endlocal & goto :EOF
