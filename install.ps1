# install.ps1
# Downloads the pre-built gccauto.exe from your GitHub Release and adds it to PATH.
# Others would run this with:
#   irm https://raw.githubusercontent.com/<you>/<repo>/main/install.ps1 | iex

$ErrorActionPreference = "Stop"

$installDir = "$env:USERPROFILE\Documents\GCCAuto"
$exeUrl     = "https://github.com/<you>/<repo>/releases/latest/download/gccauto.exe"
$exePath    = Join-Path $installDir "gccauto.exe"

New-Item -ItemType Directory -Force -Path $installDir | Out-Null

Write-Host "Downloading gccauto.exe..."
Invoke-WebRequest -Uri $exeUrl -OutFile $exePath

$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($currentPath -notlike "*$installDir*") {
    Write-Host "Adding $installDir to PATH..."
    [Environment]::SetEnvironmentVariable("Path", "$currentPath;$installDir", "User")
}

Write-Host "Installed. Open a new terminal and run: gccauto setup"