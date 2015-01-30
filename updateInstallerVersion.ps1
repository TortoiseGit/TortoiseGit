# Use Bypass so script does not need to be digitally signed
Set-ExecutionPolicy Bypass -Force

$version=$args[0]
$versionRegex = '\s*\d+.\d+.\d+.\d+\s*'
$scriptDir =  Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

# Relative file paths
$header = "$($scriptDir)/src/version.h"
$versionNumberInclude = "$($scriptDir)/src/TortoiseSISetup/VersionNumberInclude.wxi"

if ($version -eq $null) {
	Write-host "Error: No version number specified as argument. Terminating..."
	exit
}

if($version -notmatch $versionRegex) {
	Write-host "Error: Invalid version number $version. Terminating..."
	exit
}

$split = $version.replace(' ', '').split('.')

# Version standard
# split[0] -> Major Version
# split[1] -> Minor Version
# split[2] -> Micro Version
# split[3] -> Build Version

# Modify version.h
(Copy-Item $header "$($header).bak")

(Get-Content $header) |
Foreach-Object {$_ -replace '^#define FILEVER.+', "#define FILEVER 			$($split[0]),$($split[1]),$($split[2]),$($split[3])"} |
Foreach-Object {$_ -replace '^#define PRODUCTVER.+', "#define PRODUCTVER 			FILEVER"} |
Foreach-Object {$_ -replace '^#define STRFILEVER.+', "#define STRFILEVER 			""$($split[0]).$($split[1]).$($split[2]).$($split[3])"""} |
Foreach-Object {$_ -replace '^#define STRPRODUCTVER.+', "#define STRPRODUCTVER 		STRFILEVER"} |
Foreach-Object {$_ -replace '^#define TGIT_VERMAJOR.+', "#define TGIT_VERMAJOR 		$($split[0])"} |
Foreach-Object {$_ -replace '^#define TGIT_VERMINOR.+', "#define TGIT_VERMINOR 		$($split[1])"} |
Foreach-Object {$_ -replace '^#define TGIT_VERMICRO.+', "#define TGIT_VERMICRO 		$($split[2])"} |
Foreach-Object {$_ -replace '^#define TGIT_VERBUILD.+', "#define TGIT_VERBUILD 		$($split[3])"} |
Out-file $header

Write-host "Modified $header to use version $version"

# Modify VersionNumberInclude.wxi
(Copy-Item $versionNumberInclude "$($versionNumberInclude).bak")

(Get-Content $versionNumberInclude) |
Foreach-Object {$_ -replace "MajorVersion=""\d*""", "MajorVersion=""$($split[0])"""} |
Foreach-Object {$_ -replace "MinorVersion=""\d*""", "MinorVersion=""$($split[1])"""} |
Foreach-Object {$_ -replace "MicroVersion=""\d*""", "MicroVersion=""$($split[2])"""} |
Foreach-Object {$_ -replace "BuildVersion=""\d*""", "BuildVersion=""$($split[3])"""} |
Out-file $versionNumberInclude

Write-host "Modified $versionNumberInclude to use version $version"