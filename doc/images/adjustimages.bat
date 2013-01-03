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
optipng.exe -o7 -quiet %1
endlocal & goto :EOF
