param([int]$Jobs=4)
$ErrorActionPreference='Stop'
Set-Location (Join-Path $PSScriptRoot '..')
function Invoke-Checked { param([string]$Tool,[string[]]$Arguments) & $Tool @Arguments; if ($LASTEXITCODE -ne 0) { throw "$Tool failed ($LASTEXITCODE)" } }
$configure=@('--preset','windows-x64')
if ($env:JUCE_SOURCE_DIR) { $configure += "-DJUCE_SOURCE_DIR=$env:JUCE_SOURCE_DIR" }
Invoke-Checked cmake $configure
Invoke-Checked cmake @('--build','--preset','windows-x64','--parallel',"$Jobs")
Invoke-Checked ctest @('--preset','windows-x64','--verbose')
New-Item -ItemType Directory -Force dist/validation | Out-Null
Invoke-Checked './build/windows-x64/AurumHostTests_artefacts/Release/AurumHostTests.exe' @('--render',"$PWD/dist/validation")
foreach ($variant in @('Master','Track')) {
 foreach ($format in @('VST3','Standalone')) {
  $dest="dist/$variant/$format"
  New-Item -ItemType Directory -Force $dest | Out-Null
  Copy-Item -Recurse -Force "build/windows-x64/Aurum${variant}_artefacts/Release/$format/*" $dest
 }
}
Invoke-Checked python @('scripts/manifest.py','dist')
