# Use Bypass so script does not need to be digitally signed
Set-ExecutionPolicy Bypass -Force

if ($Env:Platform -eq "Win32") {
    Write-host "Using Win32: Don't need to download dll. Exiting script."
    exit
}

$apiUrl = 'https://ci.appveyor.com/api'
$headers = @{
  "Authorization" = "Bearer $Env:secureToken"
  "ContentType" = "application/json"
}

$buildVersion = $Env:APPVEYOR_BUILD_VERSION
$accountName = "gisraeli"
$projectSlug = "TortoiseSI"

# determine root directory
$root = Resolve-Path .
$downloadLocation = "$root\bin\Release\bin\"

# get project based on current build version
$project = Invoke-RestMethod -Method Get -Uri "$apiURL/projects/$accountName/$projectSlug/build/$buildVersion" -Headers $headers

# get job id for Win32 job for current build
$jobId = $project.build.jobs[0].jobId

# get artifacts from Win32 job
$artifacts = Invoke-RestMethod -Method Get -Uri "$apiUrl/buildJobs/$jobId/artifacts" -Headers $headers

# specify artifact file name and copy location
$artifactFileName = "TortoiseSI32.dll"
$localArtifactPath = "$downloadLocation\$artifactFileName"

# if download location doesn't exist, create it
if (!(Test-Path -Path $downloadLocation)) {
    Write-host "creating $downloadLocation"
    New-Item -ItemType directory -Path $downloadLocation
}

# download TortoiseSI32.dll to download location
Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jobId/artifacts/$artifactFileName" `
     -OutFile $localArtifactPath -Headers $headers

Write-host "Copied $artifactFileName to $downloadLocation"
