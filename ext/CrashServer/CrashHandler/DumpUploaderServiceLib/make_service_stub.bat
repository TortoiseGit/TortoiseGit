SET DumpUploaderHeader=DumpUploader.h
rem SET DumpUploaderUrl=%~dp0\DumpUploader3.wsdl
SET DumpUploaderUrl=http://localhost:59175/Service/DumpUploader3.svc?WSDL
SET DumpUploaderPath=%~dp0\generated
SET GsoapPath=%~dp0\..\..\external\gsoap-2.8.17\gsoap
SET TypemapDatPath=%~dp0\typemap.dat

if not exist "%DumpUploaderPath%" md "%DumpUploaderPath%"
cd "%DumpUploaderPath%"

"%GsoapPath%\bin\win32\wsdl2h.exe" -t "%TypemapDatPath%" -o "%DumpUploaderHeader%" "%DumpUploaderUrl%"
"%GsoapPath%\bin\win32\soapcpp2.exe" -2 -i -C -x "%DumpUploaderHeader%" "-I%GsoapPath%\import"

:: - and . is prohibited symbols in C++ names and gSOAP converts "-" to "_", "_" to "_USCORE" and "." to "_x002e"
:: This looks ugly and will be changed all to "_".
:: But this should be done after soapcpp2 because it needs original ids to make correct back convertion
del soapCustomBinding_DumpUploadService3_UploaderProxy.cpp
ren soapCustomBinding_USCOREDumpUploadService3_x002eUploaderProxy.cpp soapCustomBinding_DumpUploadService3_UploaderProxy.cpp
del soapCustomBinding_DumpUploadService3_UploaderProxy.h
ren soapCustomBinding_USCOREDumpUploadService3_x002eUploaderProxy.h   soapCustomBinding_DumpUploadService3_UploaderProxy.h
del CustomBinding_DumpUploadService3_Uploader.nsmap
ren CustomBinding_USCOREDumpUploadService3_x002eUploader.nsmap        CustomBinding_DumpUploadService3_Uploader.nsmap

cscript "%~dp0\replace_text.vbs" soapC.cpp
cscript "%~dp0\replace_text.vbs" soapCustomBinding_DumpUploadService3_UploaderProxy.cpp
cscript "%~dp0\replace_text.vbs" DumpUploader.h
cscript "%~dp0\replace_text.vbs" soapCustomBinding_DumpUploadService3_UploaderProxy.h
cscript "%~dp0\replace_text.vbs" soapH.h
cscript "%~dp0\replace_text.vbs" soapStub.h
cscript "%~dp0\replace_text.vbs" CustomBinding_DumpUploadService3_Uploader.nsmap

