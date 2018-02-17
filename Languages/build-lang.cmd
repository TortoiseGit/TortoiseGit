@echo off
if [%1] == [] goto missingparam
rem strip quotes
set OUTDIR=%~1
rem strip trailing baskslash
if %OUTDIR:~-1%==\ set OUTDIR=%OUTDIR:~0,-1%
set CLEAN=
if [%2]==[clean] set CLEAN="clean"

call :nmake 0130 oc
call :nmake 1026 bg
call :nmake 1027 ca
call :nmake 1028 zh_TW
call :nmake 1029 cs
call :nmake 1030 da
call :nmake 1031 de
call :nmake 1032 el
call :nmake 1034 es
call :nmake 1035 fi
call :nmake 1036 fr
call :nmake 1038 hu
call :nmake 1040 it
call :nmake 1041 ja
call :nmake 1042 ko
call :nmake 1043 nl
call :nmake 1045 pl
call :nmake 1046 pt_BR
call :nmake 1048 ro
call :nmake 1049 ru
call :nmake 1051 sk
call :nmake 1053 sv
call :nmake 1055 tr
call :nmake 1057 id
call :nmake 1058 uk
call :nmake 1065 fa
call :nmake 1066 vi
call :nmake 2052 zh_CN
call :nmake 2070 pt_PT
call :nmake 2074 sr@latin
goto :eof

:nmake
nmake /nologo /f Makefile %CLEAN% "outdir=%OUTDIR%" LANGID=%1 ISO=%2
goto :eof

:missingparam
echo Missing parameter:
echo %~nx0 [output path]
pause
