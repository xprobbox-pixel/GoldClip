$ErrorActionPreference='Stop'
if($env:GITHUB_ACTIONS-ne 'true'){throw 'Installer installation/uninstallation test runs only on ephemeral CI runners'}
$installer=(Get-ChildItem dist/installers/*Setup.exe).FullName
$target=Join-Path $env:RUNNER_TEMP 'AurumInstallerTest'
$p=Start-Process $installer -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$target`"") -Wait -PassThru
if($p.ExitCode-ne 0){throw "Installer failed: $($p.ExitCode)"}
foreach($variant in @('Master','Track')){
 $app=Join-Path $target "$variant/AURUM CLIP $variant.exe"
 if(-not(Test-Path $app)){throw 'Installed standalone is missing'}
 if((Get-FileHash $app).Hash-ne (Get-FileHash "dist/$variant/Standalone/AURUM CLIP $variant.exe").Hash){throw 'Installed standalone checksum mismatch'}
 $vst=Join-Path ${env:CommonProgramFiles} "VST3/AURUM CLIP $variant.vst3/Contents/x86_64-win/AURUM CLIP $variant.vst3"
 if(-not(Test-Path $vst)){throw 'Installed VST3 is missing'}
 if((Get-FileHash $vst).Hash-ne (Get-FileHash "dist/$variant/VST3/AURUM CLIP $variant.vst3/Contents/x86_64-win/AURUM CLIP $variant.vst3").Hash){throw 'Installed VST3 checksum mismatch'}
}
$uninstaller=Join-Path $target 'unins000.exe'
$p=Start-Process $uninstaller -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -Wait -PassThru
if($p.ExitCode-ne 0){throw 'Uninstaller failed'}
foreach($variant in @('Master','Track')){if(Test-Path "${env:CommonProgramFiles}/VST3/AURUM CLIP $variant.vst3"){throw 'VST3 remains after uninstall'}}
Write-Host 'Installer installation, payload checksums and uninstall passed.'
