param()
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
& (Join-Path $PSScriptRoot 'build-pressure-diagnostic.ps1')
$kit = 'C:\Program Files (x86)\Windows Kits\10'
$verify = Join-Path $kit 'Tools\10.0.26100.0\x64\InfVerif.exe'
$catTool = Join-Path $kit 'bin\10.0.26100.0\x86\Inf2Cat.exe'
$sign = Join-Path $kit 'bin\10.0.26100.0\x64\signtool.exe'
$thumb = 'CA50595A5BC03E62BF384B5DC39660D1E9210A9B'
$cert = Get-Item -LiteralPath "Cert:\CurrentUser\My\$thumb"
if (-not $cert.HasPrivateKey) { throw 'Certificado pessoal indisponível.' }
$inf = Join-Path $repo 'driver\GalaxyPenDiagnostic.inf'
& $verify /u $inf
if ($LASTEXITCODE -ne 0) { throw 'INF não passou validação universal.' }
& $verify /k $inf
if ($LASTEXITCODE -ne 0) { throw 'INF não passou validação kernel.' }
# A fresh, timestamped package preserves any previous signed preparation.
$package = Join-Path $repo ('build-diagnostic\packages\candidate-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $package | Out-Null
Copy-Item -LiteralPath $inf -Destination $package
foreach ($role in @('A','B')) {
    $destination = Join-Path $package "GalaxyPenDiag$role.sys"
    Copy-Item -LiteralPath (Join-Path $repo "build-diagnostic\driver\$role\GalaxyPenDiag$role.sys") -Destination $destination
    & $sign sign /fd SHA256 /sha1 $thumb /s My $destination
    if ($LASTEXITCODE -ne 0) { throw 'Falha de assinatura do observador.' }
}
Copy-Item -LiteralPath (Join-Path $repo 'build-diagnostic\GalaxyPenDiagnostic.exe') -Destination $package
& $catTool "/driver:$package" /os:10_X64
if ($LASTEXITCODE -ne 0) { throw 'Inf2Cat falhou.' }
$catalog = Join-Path $package 'GalaxyPenDiagnostic.cat'
& $sign sign /fd SHA256 /sha1 $thumb /s My $catalog
if ($LASTEXITCODE -ne 0) { throw 'Assinatura do catálogo falhou.' }
foreach ($name in @('GalaxyPenDiagA.sys','GalaxyPenDiagB.sys','GalaxyPenDiagnostic.inf')) {
    & $sign verify /pa /c $catalog (Join-Path $package $name)
    if ($LASTEXITCODE -ne 0) { throw "Catálogo não validou $name." }
}
$files = @('GalaxyPenDiagA.sys','GalaxyPenDiagB.sys','GalaxyPenDiagnostic.inf','GalaxyPenDiagnostic.cat','GalaxyPenDiagnostic.exe')
[ordered]@{
    Kind='PressureDiagnostic'; DriverVersion='0.1.0.0'; Protocol=1; Signed=$true
    Installed=$false; HardwareValidated=$false; Attachment='Explicit per-instance legacy UpperFilters with live stack gate'
    Files=@($files | ForEach-Object { @{Name=$_;SHA256=(Get-FileHash -LiteralPath (Join-Path $package $_) -Algorithm SHA256).Hash} })
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $package 'manifest.json') -Encoding utf8
Write-Host "Candidato assinado, NÃO instalado: $package"
$package
