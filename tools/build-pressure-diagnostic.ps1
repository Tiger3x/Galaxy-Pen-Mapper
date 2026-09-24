param()
# Preparation only. No INF, signing, certificate import or installation is produced.
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $repo 'build-diagnostic'
$vs = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools'
$dev = Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
$msbuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$command = 'call "{0}" -arch=x64 >nul && "{1}" -S "{2}" -B "{3}" -G Ninja -DCMAKE_BUILD_TYPE=Release && "{1}" --build "{3}"' -f $dev,$cmake,(Join-Path $repo 'diagnostics'),$build
$buildOutput = & cmd.exe /d /s /c $command 2>&1
$buildExit = $LASTEXITCODE
$buildOutput | Set-Content -LiteralPath (Join-Path $build 'ninja-build.log')
if ($buildExit -ne 0) {
    $buildOutput | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }
    throw 'Compilação do diagnóstico falhou.'
}
& $ctest --test-dir $build --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Testes do diagnóstico falharam.' }
foreach ($role in @('A','B')) {
    & $msbuild (Join-Path $repo 'driver\GalaxyPenDiagnostic.vcxproj') /t:Rebuild "/p:Configuration=$role" /p:Platform=x64 /p:RunCodeAnalysis=true /m /v:minimal
    if ($LASTEXITCODE -ne 0) { throw "Compilação WDK $role falhou." }
    $analysisPath = Join-Path $build "obj\$role\DiagnosticProbe.nativecodeanalysis.xml"
    [xml]$analysis = Get-Content -Raw -LiteralPath $analysisPath
    if ($analysis.SelectNodes('//DEFECT').Count) { throw "Análise estática $role encontrou defeitos." }
}
$files = @('GalaxyPenDiagnostic.exe','DiagnosticTest.exe','driver\A\GalaxyPenDiagA.sys','driver\B\GalaxyPenDiagB.sys')
[ordered]@{
    Purpose = 'Offline preparation only; placement/deployment unresolved'
    Installable = $false
    Installed = $false
    Signed = $false
    HardwareValidated = $false
    Protocol = 1
    BuiltUtc = [DateTime]::UtcNow.ToString('o')
    Files = @($files | ForEach-Object { @{ Name = $_; SHA256 = (Get-FileHash -LiteralPath (Join-Path $build $_) -Algorithm SHA256).Hash } })
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $build 'preparation-manifest.json') -Encoding utf8
Write-Host 'Diagnóstico compilado e testado offline. Não instalável: sem INF, assinatura ou alterações no sistema.'
