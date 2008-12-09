@echo off
rem Copyright (C) 2004-2008 the TortoiseSVN team
rem This file is distributed under the same license as TortoiseSVN

rem Last commit by:
rem $Author: luebbe $
rem $Date: 2008-09-29 19:22:31 +0800 (Mon, 29 Sep 2008) $
rem $Rev: 14138 $

SETLOCAL
FOR /F "usebackq skip=1" %%c IN (`msgfmt %* 2^>^&1 ^| grep -c msgstr`) DO SET /A count=%%c

if Defined count (
  rem
) else (
  SET count=0
)
echo %count%
ENDLOCAL
