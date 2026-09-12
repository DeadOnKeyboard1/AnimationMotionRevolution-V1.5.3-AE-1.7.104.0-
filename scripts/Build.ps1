[CmdletBinding()]
param([string]$VcpkgRoot = $env:VCPKG_ROOT)
$ErrorActionPreference = 'Stop'
if (!$VcpkgRoot -or !(Test-Path "$VcpkgRoot/scripts/buildsystems/vcpkg.cmake")) {
    throw 'Set VCPKG_ROOT or pass -VcpkgRoot to your bootstrapped vcpkg checkout.'
}
$env:VCPKG_ROOT = (Resolve-Path $VcpkgRoot).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Install Visual Studio C++ desktop build tools and a Windows SDK.' }
$dev = Join-Path $vs 'Common7/Tools/VsDevCmd.bat'
$lines = & cmd /d /s /c "`"$dev`" -arch=x64 -host_arch=x64 >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'Developer environment setup failed' }
foreach ($line in $lines) {
    $index = $line.IndexOf('=')
    if ($index -gt 0) { [Environment]::SetEnvironmentVariable($line.Substring(0,$index),$line.Substring($index+1),'Process') }
}
Push-Location (Join-Path $PSScriptRoot '..')
try {
    & cmake --preset release
    if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
    & cmake --build --preset release
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
} finally { Pop-Location }
