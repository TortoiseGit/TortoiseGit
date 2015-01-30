# Use Bypass so script does not need to be digitally signed
Set-ExecutionPolicy Bypass -Force

# If building Win32 version, upload TortoiseSI32.dll
if($Env:Platform -eq "Win32")
{
	Write-host "Uploading TortoiseSI32.dll"
	Push-AppveyorArtifact "\bin\Release\bin\TortoiseSI32.dll"
}
# if building x64 version, upload installer
else if($Env:Platform -eq "x64")
{
	Write-host "Uploading installer"
	Push-AppveyorArtifact "\bin\setup\x64\TortoiseSI.msi"
}
else
{
	$platform = $Env:Platform
	Write-host "No artifacts defined for platform $platform"
}