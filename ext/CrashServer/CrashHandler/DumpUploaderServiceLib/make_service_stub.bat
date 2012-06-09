SET DumpUploaderHeader=DumpUploader.h
SET DumpUploaderUrl=https://www.crash-server.com/crashserver/DumpUploader.asmx?WSDL
rem SET DumpUploaderUrl=http://localhost:59175/DumpUploader.asmx?WSDL
SET DumpUploaderPath=%~dp0\generated
SET GsoapPath=%~dp0\..\..\external\gsoap-win32-2.8.0
SET TypemapDatPath=%~dp0\typemap.dat

if not exist "%DumpUploaderPath%" md "%DumpUploaderPath%"
cd "%DumpUploaderPath%"

"%GsoapPath%\bin\win32\wsdl2h.exe" -t "%TypemapDatPath%" -o "%DumpUploaderHeader%" "%DumpUploaderUrl%"
"%GsoapPath%\bin\win32\soapcpp2.exe" -2 -i -C -x "%DumpUploaderHeader%" "-I%GsoapPath%\import"
