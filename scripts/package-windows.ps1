param([string]$Stage='dist')
$ErrorActionPreference='Stop'
Set-Location "$PSScriptRoot/.."
$identity=Get-Content cmake/Identity.cmake -Raw
$version=[regex]::Match($identity,'set\(AURUM_VERSION ([^)]+)\)').Groups[1].Value
$company=[regex]::Match($identity,'set\(AURUM_MANUFACTURER "([^"]+)"\)').Groups[1].Value
$stagePath=(Resolve-Path $Stage).Path
$output=Join-Path $stagePath 'installers';New-Item -ItemType Directory -Force $output | Out-Null
$iscc=(Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source
if(-not $iscc){$iscc='C:\Program Files (x86)\Inno Setup 6\ISCC.exe'}
if(-not(Test-Path $iscc)){throw 'Inno Setup 6 is required; install it from https://jrsoftware.org/isinfo.php'}
function Sign-File([string]$file){
 if($env:AURUM_SIGN_PFX){
  $tool=(Get-Command signtool.exe -ErrorAction Stop).Source
  & $tool sign /fd SHA256 /tr 'http://timestamp.digicert.com' /td SHA256 /f $env:AURUM_SIGN_PFX /p $env:AURUM_SIGN_PASSWORD $file
  if($LASTEXITCODE-ne 0){throw 'Authenticode signing failed'}
  & $tool verify /pa $file;if($LASTEXITCODE-ne 0){throw 'Signature verification failed'}
 }
}
Get-ChildItem $stagePath -Recurse -File | Where-Object {$_.Extension-in @('.exe','.vst3','.dll') -and $_.FullName-notlike '*\installers\*'} | ForEach-Object {Sign-File $_.FullName}
& $iscc "/DAppVersion=$version" "/DCompanyName=$company" "/DStageDir=$stagePath" "/DOutputDir=$output" packaging/windows/AurumClip.iss
if($LASTEXITCODE-ne 0){throw 'Inno Setup compilation failed'}
$installer=Join-Path $output "AURUM-CLIP-$version-Windows-x64-Setup.exe"
Sign-File $installer
python scripts/manifest.py $Stage
if($LASTEXITCODE-ne 0){throw 'Manifest generation failed'}
Write-Host "Created $installer; consult manifest for signing status."
