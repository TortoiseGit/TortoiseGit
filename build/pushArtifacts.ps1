# Use Bypass so script does not need to be digitally signed
Set-ExecutionPolicy Bypass -Force

# If building Win32 version, upload TortoiseSI32.dll
if($Env:Platform -eq "Win32")
{
	$root = Resolve-Path .
	Push-AppveyorArtifact "$root\bin\Release\bin\TortoiseSI32.dll"
	Push-AppveyorArtifact "$root\bin\Release\bin\TortoiseSIStub32.dll"
	Write-host "Uploaded TortoiseSI32.dll and TortoiseSIStub32.dll"
}
# if building x64 version, upload installer
elseif($Env:Platform -eq "x64")
{
	$root = Resolve-Path .
	Push-AppveyorArtifact "$root\bin\setup\x64\TortoiseSI.msi"
	Write-host "Uploading TortoiseSI installer"
}
else
{
	$platform = $Env:Platform
	Write-host "No artifacts defined for platform $platform"
}