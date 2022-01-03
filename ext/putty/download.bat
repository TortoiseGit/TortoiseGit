@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
pushd %~dp0

rem recheck whether we need to re-download
certUtil -hashfile download.txt sha256 > lastrun.tmp
fc /C lastrun.tmp lastrun.txt || (
  del lastrun.txt
  move lastrun.tmp lastrun.txt
  set needsupdate=1
)
del lastrun.tmp

rem do the download and checks
for /F "tokens=1-3" %%a in (download.txt) do (
  echo %%a
  if "%needsupdate%"=="1" del "%%b"
  if not exist "%%b" (
    set LOCALFILENAME="%%b.tmp"
    del !LOCALFILENAME!
    curl --fail-early --output !LOCALFILENAME! "%%a" || (
      echo "Download of %%a failed."
      goto error
    )
  ) else (
    set LOCALFILENAME="%%b"
  )
  if not exist !LOCALFILENAME! (
    echo "File !LOCALFILENAME! is missing."
    goto error
  )
  if "%%c"=="" goto error
  certUtil -hashfile !LOCALFILENAME! sha256 | findstr /I %%c > findstroutput.tmp
  echo %%c> wantedhash.tmp
  fc /C wantedhash.tmp findstroutput.tmp && (
    if NOT !LOCALFILENAME!=="%%b" move "!LOCALFILENAME!" "%%b"
    del findstroutput.tmp wantedhash.tmp
  ) || (
    echo "Invalid hash for !LOCALFILENAME! found!"
    del !LOCALFILENAME! findstroutput.tmp wantedhash.tmp
    goto error
  )
)

popd
goto :eof

:error
popd
pause
exit /b 1
