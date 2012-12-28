@echo off
if "%1"=="" goto missingparam
set OUTDIR=%1
set CLEAN=""
if "%2"=="clean" set CLEAN="clean"

call :nmake 1028 zh_TW
call :nmake 1029 cs
call :nmake 1031 de
call :nmake 1034 es_ES
call :nmake 1036 fr
call :nmake 1041 ja
call :nmake 1043 nl_NL
call :nmake 1045 pl_PL
call :nmake 1046 pt_BR
call :nmake 1049 ru
call :nmake 1053 sv
call :nmake 1055 tr
call :nmake 2052 zh_CN
goto :eof

:nmake
nmake /f Makefile %CLEAN% outdir=%OUTDIR% LANGID=%1 ISO=%2
goto :eof

:missingparam
echo Missing parameter:
echo %~nx0 [output path]
pause
