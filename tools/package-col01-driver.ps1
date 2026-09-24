param(
    [ValidateSet('Probe', 'Corrector', 'Both')][string]$Variant = 'Probe',
    [string]$CertificateThumbprint = 'CA50595A5BC03E62BF384B5DC39660D1E9210A9B',
    [string]$BuildDirectory
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildDir = if($BuildDirectory){[IO.Path]::GetFullPath($BuildDirectory)}else{Join-Path $repoRoot 'build-ninja'}
$vsRoot = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools'
$vsDevCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
$cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
$infVerif = 'C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\InfVerif.exe'
$inf2cat = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe'
$signtool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
foreach ($path in @($vsDevCmd, $msbuild, $cmake, $ctest, $infVerif, $inf2cat, $signtool)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Ferramenta necessária não encontrada: $path" }
}
$certificate = Get-ChildItem Cert:\CurrentUser\My | Where-Object Thumbprint -eq $CertificateThumbprint
if (-not $certificate -or -not $certificate.HasPrivateKey) { throw 'Certificado de teste pessoal indisponível.' }

New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
$command = 'call "{0}" -arch=x64 >nul && "{1}" -S "{3}" -B "{2}" -G Ninja -DCMAKE_BUILD_TYPE=Release && "{1}" --build "{2}" -j 8' -f $vsDevCmd, $cmake, $buildDir, $repoRoot
$output = & cmd.exe /d /s /c $command 2>&1
$buildExit = $LASTEXITCODE
$output | Set-Content -LiteralPath (Join-Path $buildDir 'col01-build.log')
if ($buildExit -ne 0) {
    $output | Select-Object -Last 35 | ForEach-Object { Write-Host $_ }
    throw 'Build Ninja falhou.'
}
& $ctest --test-dir $buildDir --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Testes falharam; nenhum pacote novo foi gerado.' }
& (Join-Path $buildDir 'GalaxyPenHidScanner.exe') --wcom-col01-report-lab
if ($LASTEXITCODE -ne 0) { throw 'Descritor COL01 não corresponde ao motor; empacotamento interrompido.' }

$variants = @(
    @{ Name = 'Probe'; Configuration = 'Probe'; Folder = 'col01-probe'; Stem = 'GalaxyPenCol01Probe' },
    @{ Name = 'Corrector'; Configuration = 'Release'; Folder = 'col01-corrector'; Stem = 'GalaxyPenCol01Corrector' }
)
foreach ($item in $variants) {
    if ($Variant -ne 'Both' -and $Variant -ne $item.Name) { continue }
    & $msbuild (Join-Path $repoRoot 'driver\GalaxyPenPassThrough.vcxproj') /t:Rebuild "/p:Configuration=$($item.Configuration)" /p:Platform=x64 /p:RunCodeAnalysis=true "/p:OutDir=$buildDir\driver\$($item.Configuration)\" "/p:IntDir=$buildDir\driver\obj\$($item.Configuration)\" /m /v:minimal
    if ($LASTEXITCODE -ne 0) { throw "Build WDK $($item.Name) falhou." }
    $analysisPath = Join-Path $buildDir "driver\obj\$($item.Configuration)\PassThrough.nativecodeanalysis.xml"
    [xml]$analysis = Get-Content -Raw -LiteralPath $analysisPath
    if ($analysis.SelectNodes('//DEFECT').Count -ne 0) { throw 'Análise estática encontrou defeitos; pacote não será assinado.' }
    $inf = Join-Path $repoRoot "driver\$($item.Stem).inf"
    & $infVerif /u $inf
    if ($LASTEXITCODE -ne 0) { throw 'InfVerif /u falhou.' }
    & $infVerif /k $inf
    if ($LASTEXITCODE -ne 0) { throw 'InfVerif /k falhou.' }
    $packageDir = Join-Path $buildDir "driver\packages\$($item.Folder)"
    New-Item -ItemType Directory -Path $packageDir -Force | Out-Null
    $sys = Join-Path $packageDir "$($item.Stem).sys"
    $cat = Join-Path $packageDir "$($item.Stem).cat"
    Copy-Item -LiteralPath (Join-Path $buildDir "driver\$($item.Configuration)\GalaxyPenPassThrough.sys") -Destination $sys -Force
    Copy-Item -LiteralPath $inf -Destination (Join-Path $packageDir "$($item.Stem).inf") -Force
    Copy-Item -LiteralPath (Join-Path $buildDir 'GalaxyPenMapperTray.exe') -Destination (Join-Path $packageDir 'GalaxyPenMapperTray.exe') -Force
    Export-Certificate -Cert $certificate -FilePath (Join-Path $packageDir 'GalaxyPenMapperTest.cer') -Force | Out-Null
    & $signtool sign /fd SHA256 /sha1 $CertificateThumbprint /s My $sys
    if ($LASTEXITCODE -ne 0) { throw 'Assinatura SYS falhou.' }
    & $inf2cat "/driver:$packageDir" /os:10_X64
    if ($LASTEXITCODE -ne 0) { throw 'Inf2Cat falhou.' }
    & $signtool sign /fd SHA256 /sha1 $CertificateThumbprint /s My $cat
    if ($LASTEXITCODE -ne 0) { throw 'Assinatura CAT falhou.' }
    & $signtool verify /pa $cat
    if ($LASTEXITCODE -ne 0) { throw 'Verificação da assinatura CAT falhou.' }
    & $signtool verify /pa /c $cat $sys
    if ($LASTEXITCODE -ne 0) { throw 'O SYS não corresponde ao catálogo assinado.' }
    & $signtool verify /pa /c $cat (Join-Path $packageDir "$($item.Stem).inf")
    if ($LASTEXITCODE -ne 0) { throw 'O INF não corresponde ao catálogo assinado.' }
    $files = @("$($item.Stem).sys", "$($item.Stem).cat", "$($item.Stem).inf", 'GalaxyPenMapperTray.exe')
    $manifest = [ordered]@{
        Variant = $item.Name; DriverVersion = '0.3.2.0'; ConfigProtocol = 4; StatusProtocol = 5
        MinSensitivityPercent = 100; MaxSensitivityPercent = 3200; DefaultSensitivityPercent = 800
        BuiltUtc = [DateTime]::UtcNow.ToString('o'); Installed = $false; LiveValidation = 'pending'
        Files = @($files | ForEach-Object { @{ Name = $_; SHA256 = (Get-FileHash -LiteralPath (Join-Path $packageDir $_) -Algorithm SHA256).Hash } })
    }
    $manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $packageDir 'build-manifest.json') -Encoding UTF8
    Write-Host "Pacote $($item.Name) verificado: $packageDir"
}
Write-Host 'Nenhum driver foi instalado; nenhuma configuração de inicialização foi alterada.'
