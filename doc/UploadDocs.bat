@echo off
rem run this batch file to upload the docs to our sourceforge server
rem to make this work, you have to create a file "docserverlogin.bat"
rem which sets the variables USERNAME and PASSWORD, and also set the PSCP
rem variable to point to your scp program the PLINK variable to the plink
rem program and the ZIP variable to your zip program

rem example docserverlogin.bat file
rem 
rem @echo off
rem set USERNAME=myname
rem set PASSWORD=mypassword
rem set PsCP="C:\Programme\PuttY\pscp.exe"
rem set PLINK="C:\Programme\Putty\plink.exe"
rem set ZIP="C:\Programme\7-zip\7z.exe"


call docserverlogin.bat

cd output
del docs.zip

%ZIP% a -r -x!*.pdf -x!*.chm -tzip docs.zip *

%PSCP% -r -l %USERNAME% -pw %PASSWORD% docs.zip www.tortoisesvn.net:/var/www/vhosts/default/htdocs/docs

if "%1"=="" (
%PLINK% www.tortoisesvn.net -l %USERNAME% -pw %PASSWORD% unzip -o /var/www/vhosts/default/htdocs/docs/docs.zip -d /var/www/vhosts/default/htdocs/docs/nightly;rm -f /var/www/vhosts/default/htdocs/docs/docs.zip
) else (
%PLINK% www.tortoisesvn.net -l %USERNAME% -pw %PASSWORD% unzip -o /var/www/vhosts/default/htdocs/docs/docs.zip -d /var/www/vhosts/default/htdocs/docs/release;rm -f /var/www/vhosts/default/htdocs/docs/docs.zip
)
del docs.zip

cd ..
