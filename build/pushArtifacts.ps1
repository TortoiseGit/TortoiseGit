# Use Bypass so script does not need to be digitally signed
Set-ExecutionPolicy Bypass -Force

$version = $Env:APPVEYOR_BUILD_VERSION

# If building Win32 version, upload TortoiseSI32.dll
if($Env:Platform -eq "Win32")
{
	$root = Resolve-Path .
	Push-AppveyorArtifact "$root\bin\Release\bin\TortoiseSI32.dll"
	Push-AppveyorArtifact "$root\bin\Release\bin\TortoiseSIStub32.dll"
	Push-AppveyorArtifact "$root\bin\setup\x64\TortoiseSI.msi" -FileName "TortoiseSI-$($version)-x86.msi"	
	Write-host "Uploaded TortoiseSI32.dll, TortoiseSIStub32.dll and 32 bit installer"
}
# if building x64 version, upload installer
elseif($Env:Platform -eq "x64")
{
	$root = Resolve-Path .
	Push-AppveyorArtifact "$root\bin\setup\x64\TortoiseSI.msi" -FileName "TortoiseSI-$($version)-x64.msi"
	Write-host "Uploading TortoiseSI 64 bit installer"
}
else
{
	$platform = $Env:Platform
	Write-host "No artifacts defined for platform $platform"
}