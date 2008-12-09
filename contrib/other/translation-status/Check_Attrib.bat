@echo off
rem Copyright (C) 2004-2008 the TortoiseSVN team
rem This file is distributed under the same license as TortoiseSVN

rem Last commit by:
rem $Author: luebbe $
rem $Date: 2008-09-29 19:22:31 +0800 (Mon, 29 Sep 2008) $
rem $Rev: 14138 $

rem Count messages in given PO File that match the given attributes 
SETLOCAL
FOR /F "usebackq skip=1" %%c IN (`msgattrib %* 2^>nul ^| grep -c msgid`) DO SET /A count=%%c
rem If count is set, then its always one too high
if Defined count (
  SET /A count -= 1
) else (
  SET count=0
)
echo %count%
ENDLOCAL
