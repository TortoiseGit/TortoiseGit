@echo off
rem Copyright (C) 2004-2008 the TortoiseSVN team
rem This file is distributed under the same license as TortoiseSVN

rem Last commit by:
rem $Author: luebbe $
rem $Date: 2008-09-29 19:22:31 +0800 (Mon, 29 Sep 2008) $
rem $Rev: 14138 $

rem Script to calculate the translation status of a single .po file
rem Parameters:
rem %1=Working directory
rem %2=Name of .po file
rem %3=Number of strings in total

SETLOCAL ENABLEDELAYEDEXPANSION

set ScriptPath=%~dp0
set Blanks20="                    "

pushd %1

if exist %2 (
  set totSVN=%3
  set errSVN=0
  set accSVN=0
  set traSVN=0
  set untSVN=0
  set fuzSVN=0

  FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Errors.bat --check %2`) DO SET errSVN=%%p
  FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Errors.bat --check-accelerators %2`) DO SET accSVN=%%p
  FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --translated --no-fuzzy --no-obsolete %2`) DO SET traSVN=%%p
  FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --only-fuzzy --no-obsolete %2`) DO SET fuzSVN=%%p
  FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --untranslated --no-obsolete %2`) DO SET untSVN=%%p

  SET /A errsumSVN=!fuzSVN!+!untSVN!+!errSVN!+!accSVN!

  if !errSVN! NEQ 0 (
    set outSVN=BROKEN
    set outStat=
  ) else if !errsumSVN! EQU 0 (
    set outSVN=OK
    set outStat=
  ) else (
    if !totSVN! EQU !traSVN! (
      set outSVN=99%%
      set outStat=- ^(!fuzSVN!/!untSVN!/!accSVN!^)
    ) else (
      set /a outTMP=100*!traSVN!/totSVN
      echo !outTMP!
      if !outTMP! LSS 10 set outTMP= !outTMP!
      set outSVN=!outTMP!%%
      set outStat=- ^(!fuzSVN!/!untSVN!/!accSVN!^)
    )
  )

) else (
  set outSVN=NONE
)

popd

set result=%outSVN% %outStat%%Blanks20:~1,20%
set result=%result:~0,19%
echo %result%

ENDLOCAL
