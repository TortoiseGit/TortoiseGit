@echo off
rem Copyright (C) 2004-2008 the TortoiseSVN team
rem This file is distributed under the same license as TortoiseSVN

rem Last commit by:
rem $Author: luebbe $
rem $Date: 2008-09-29 19:22:31 +0800 (Mon, 29 Sep 2008) $
rem $Rev: 14138 $

rem Script to calculate the GUI and DOC translation status report for TortoiseSVN

SETLOCAL ENABLEDELAYEDEXPANSION

rem Trunk and branch location. 
rem Without slash, because they're not only used for directories
set Trunk=trunk
set Brnch=branches\1.5.x

rem Paths & working directories
set ScriptPath=%~dp0
set RootDir=..\..\..\..\
set LanguageList=%RootDir%%Trunk%\Languages\Languages.txt

rem Some blanks for formatting
set Blanks30="                              "
set Sep75============================================================================

rem Get current revision of working copy
for /F "usebackq" %%p in (`svnversion`) do set WCRev=%%p

rem ----------------------------------------
rem Set parameters for gui translation
rem ----------------------------------------
set WDirTrunk=%RootDir%%Trunk%\Languages
set WDirBrnch=%RootDir%%Brnch%\Languages
set LogFile=%ScriptPath%\gui_translation.txt

echo.> %LogFile%
echo TortoiseSVN GUI translation status for revision !WCRev:~0,5!^ >> %LogFile%

echo.>> %LogFile%
call :CheckTranslation Tortoise TortoiseGUI


rem ----------------------------------------
rem Set parameters for doc translation
rem ----------------------------------------
set WDirTrunk=%RootDir%%Trunk%\doc\po
set WDirBrnch=%RootDir%%Brnch%\doc\po
set LogFile=%ScriptPath%\doc_translation.txt

echo.> %LogFile%
echo TortoiseSVN DOC translation status for revision !WCRev:~0,5!^ >> %LogFile%

echo.>> %LogFile%
call :CheckTranslation TortoiseSVN TortoiseSVN

echo.>> %LogFile%
echo.>> %LogFile%
call :CheckTranslation TortoiseMerge TortoiseMerge

ENDLOCAL
Exit /b 0
rem End of Program
rem ########################################


:CheckTranslation
rem ----------------------------------------------------------------------
rem Subroutine to check the translation status 
rem %1 = Name of po/pot files
rem %2 = Caption for log file
rem ----------------------------------------------------------------------

call :Prepare %WDirTrunk% %1 trunk
set TotalTrunk=%Errorlevel%
call :Prepare %WDirBrnch% %1 branch
set TotalBrnch=%Errorlevel%

set Cap=%2 %Sep75%
set Cap=!Cap:~0,75!
echo !Cap! >> %LogFile%

rem Write log file header 
echo                                : Developer Version   : Current Release >> %LogFile%
echo                  Location      : %Trunk%               : %Brnch% >> %LogFile%
echo                  Total strings : %TotalTrunk%                : %TotalBrnch% >> %LogFile%
echo Language                       : Status (fu/un/ma)   : Status (fu/un/ma) >> %LogFile% 
echo =========================================================================== >> %LogFile%

rem Let's loop through all trunk translations.
rem Don't care if there's a language more on the release branch (dead language anyway)  
rem !!! ATTENTION 
rem !!! There is a real TAB key inside "delims=	;"
rem !!! Please leave it there

for /F "eol=# delims=	; tokens=1,5" %%i in (%LanguageList%) do (
  set PoFile=%1_%%i.po
  set LangName=%%j ^(%%i^)%Blanks30:~1,30%
  set LangName=!LangName:~0,30!

  if exist trunk\!PoFile! (
    echo Computing Status for !LANGNAME!
    for /F "usebackq delims=#" %%p in (`Check_Status.bat trunk !PoFile! !TotalTrunk!`) do set StatusTrunk=%%p
    for /F "usebackq delims=#" %%p in (`Check_Status.bat branch !PoFile! !TotalBrnch!`) do set StatusBrnch=%%p
    echo !LANGNAME! : !StatusTrunk! : !StatusBrnch! >> %Logfile%
  )
)

rem Write log file footer 
echo =========================================================================== >> %LogFile%
echo Status: fu=fuzzy - un=untranslated - ma=missing accelerator keys >> %LogFile%
echo =========================================================================== >> %LogFile%

call :Cleanup %1 trunk
call :Cleanup %1 branch

exit /b 0
rem End:CheckTranslation
rem ----------------------------------------------------------------------


:Prepare
rem ----------------------------------------------------------------------
rem Subroutine to prepare the working directory for the check 
rem %1 = Source dir
rem %2 = File name(s) to copy
rem %3 = Dest dir
rem Return number of strings in errorlevel
rem ----------------------------------------------------------------------
echo.
echo Preparing working directory %2 %3
echo ----------------------------------------------------------------------

copy %1\%2*.po %3 /Y 1>NUL
copy %1\%2.pot %3 /Y 1>NUL

pushd %3

FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat %2.pot`) DO SET StringsTotal=%%p

FOR %%i in (%2*.po) DO (
  echo %%i
  msgmerge --no-wrap --quiet --no-fuzzy-matching -s %%i %2.pot -o %%i 2> NUL
)
popd

exit /b !StringsTotal!
rem End:Prepare
rem ----------------------------------------------------------------------


:Cleanup
rem ----------------------------------------------------------------------
rem Subroutine to clean up the working directory after the check 
rem %1 = File name(s) to delete
rem %2 = Directory
rem ----------------------------------------------------------------------
echo Cleaning up working directory %1 %2
pushd %2
del %1*.po /Q 2>Nul
del %1.pot /Q 2>Nul
del %1*.mo /Q 2>Nul
popd
exit /b 0
rem End:Cleanup
rem ----------------------------------------------------------------------
