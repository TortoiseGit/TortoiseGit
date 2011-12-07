@echo off & setlocal enableextensions enabledelayedexpansion
:: ============================================================ 
:: $Id: adjustimages.bat 22184 2011-10-25 15:02:18Z luebbe.tortoisesvn $
:: ============================================================ 
:: This script verifies all .png images to see if they fit
:: in the PDF version of the documentation. The FOP version we
:: use doesn't scale images automatically when they are too
:: wide or high.
:: We want images to be at most 5.75 inches wide and 9 inches
:: high.
:: The script retrieves the DPI setting in the image and
:: and calculates the actual size of the image. If it exceeds
:: the limits, the DPI setting of the image is increased to
:: fit the limits.
::
:: By default all images in all languages will be checked.
:: You can specify a group of files to check using the first
:: parameter, e.g.
::     adjustimages en\Rev*.png
::
:: The image manipulation is done using NConvert.exe from
:: http://www.xnview.com
:: ============================================================ 
:: Set image size limits in 1000ths of an inch
set /a w_limit = 5750
set /a h_limit = 9000
::
::
if '%1' == '' goto :DoDefault
for %%? in (%1) do Call :ProcAdjustFile %%?
endlocal & goto:EOF
::
:DoDefault
for %%? in (en\*.png) do Call :ProcAdjustFile %%?
:: Cleanup
for %%? in (f_info.txt) do if exist %%? del %%?
endlocal & goto :EOF
::===============================================================
:ProcAdjustFile FileName
:: Extract resolution info from file
setlocal enableextensions enabledelayedexpansion
nconvert.exe -info %1>f_info.txt
::
:: Extract image width, height and dpi
:: Width   = 3rd word on 10th line
:: Height  = 3rd word on 11th line
:: Channel = 5th word on 12th line
:: XDPI    = 3rd word on 19th line
:: YDPI    = 3rd word on 20th line
:: Do the test. Get the third word of the tenth line.
call :ProcGetLine f_info.txt 10 getLine
for /f "tokens=3" %%? in ("%getLine%") do set /a w_image = %%?
call :ProcGetLine f_info.txt 11 getLine
for /f "tokens=3" %%? in ("%getLine%") do set /a h_image = %%?
call :ProcGetLine f_info.txt 12 getLine
for /f "tokens=5" %%? in ("%getLine%") do set /a channels = %%?
call :ProcGetLine f_info.txt 19 getLine
for /f "tokens=3" %%? in ("%getLine%") do set /a xdpi = %%?
call :ProcGetLine f_info.txt 20 getLine
for /f "tokens=3" %%? in ("%getLine%") do set /a ydpi = %%?
:: Set default dpi if no dpi was found
set /a must_convert = 0
set /a must_channel = 0
if %xdpi% equ 0 (
   set /a xdpi = 96
   set /a must_convert = 1
)
if %ydpi% equ 0 (
   set /a ydpi = 96
   set /a must_convert = 1
)
if %channels% geq 4 (
    echo %1: alpha channel detected
    set /a must_channel = 1
    set /a must_convert = 1
)
::
:: Calculate image width and height (factor 1000 is used because
:: of the integer math.
set /a w_image_inch = w_image * 1000 / xdpi
set /a h_image_inch = h_image * 1000 / ydpi
rem echo Image (w - h): %w_image% - %h_image%
rem echo DPI (x - y)  : %xdpi% - %ydpi%
rem echo Size (w - h) : %w_image_inch% - %h_image_inch%
set /a w_delta = w_limit - w_image_inch
set /a h_delta = h_limit - h_image_inch
rem echo Delta (w - h): %w_delta% - %h_delta%
:: If height and width are within limits, we're done
:: If not, determine direction with largest overdraw and
::   calculate a dpi setting that will fit the image into the
::   available space
::   Calculation method: new_dpi = (pixels * 1000) / limit
if %w_delta% geq 0 (
   if %h_delta% geq 0 (
      if %must_convert% equ 1 (
         echo %1: image fits with default dpi
         set /a new_dpi = 95
      ) else (
         echo %1: image is ok
         goto :Done
      )
   ) else (
      echo %1: image too high
      set /a new_dpi = h_image * 1000 / h_limit
   )
) else (
   if %h_delta% geq 0 (
      echo %1: image too wide
      set /a new_dpi = w_image * 1000 / w_limit
   ) else (
      echo %1: image too wide and too high
      set /a new_xdpi = w_image * 1000 / w_limit
      set /a new_ydpi = h_image * 1000 / h_limit
      if !new_xdpi! leq !new_ydpi! (
         set /a new_dpi = !new_ydpi!
      ) else (
         set /a new_dpi = !new_xdpi!
      )
      rem echo dpi required for width !new_xdpi!
      rem echo dpi required for height !new_ydpi!
   )
)
:: Make sure the dpi is large enough (integer arithmetic truncates)
set /a new_dpi = new_dpi + 1
echo adjust dpi to %new_dpi%
nconvert.exe -o %1 -dpi %new_dpi% %1 >nul
:Done
if %must_channel% equ 1 (
    nconvert.exe -o %1 -ctype rgb %1 >nul
)
optipng.exe -o7 -quiet %1
endlocal & goto :EOF
::===============================================================
:ProcGetLine FileName LineNro returnText
setlocal enableextensions
set lineNro_=%2
set /a lineNro_-=1
set return_=
for /f "tokens=* skip=%lineNro_% delims=" %%r in ('type %1') do (
  if not defined return_ set return_=%%r)
endlocal & set %3=%return_% & goto :EOF
