# install.ps1
# Downloads the pre-built gccauto.exe from the GitHub Release and adds it to PATH.
# Others install it with:
#   irm https://raw.githubusercontent.com/ubay-mostafa/gccauto/main/install.ps1 | iex

$ErrorActionPreference = "Stop"

$installDir = "$env:USERPROFILE\Documents\GCCAuto"
$exeUrl     = "https://github.com/ubay-mostafa/gccauto/releases/latest/download/gccauto.exe"
$exePath    = Join-Path $installDir "gccauto.exe"

New-Item -ItemType Directory -Force -Path $installDir | Out-Null

Write-Host "Downloading gccauto.exe..."
Invoke-WebRequest -Uri $exeUrl -OutFile $exePath

$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($currentPath -notlike "*$installDir*") {
    Write-Host "Adding $installDir to PATH..."
    [Environment]::SetEnvironmentVariable("Path", "$currentPath;$installDir", "User")
}

# Also update THIS session's PATH, so gccauto works right away without opening a new window
if ($env:Path -notlike "*$installDir*") {
    $env:Path += ";$installDir"
}

Write-Host "Installed. You can run 'gccauto setup' right now in this window."