param(
    [ValidateSet('Probe', 'Corrector')][string]$Variant = 'Probe',
    [switch]$Install,
    [switch]$Update,
    [string]$PackageDirectory
)

# Check-only by default. Never reboots, modifies boot policy or imports a certificate.
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$folder = if ($Variant -eq 'Probe') { 'col01-probe' } else { 'col01-corrector' }
$stem = if ($Variant -eq 'Probe') { 'GalaxyPenCol01Probe' } else { 'GalaxyPenCol01Corrector' }
$package = if($PackageDirectory){(Resolve-Path -LiteralPath $PackageDirectory).Path}else{Join-Path $repoRoot "build-ninja\driver\packages\$folder"}
$manifest = Get-Content -Raw -LiteralPath (Join-Path $package 'build-manifest.json') | ConvertFrom-Json
if ($manifest.Variant -ne $Variant -or $manifest.DriverVersion -ne '0.3.2.0' -or
    $manifest.ConfigProtocol -ne 4 -or $manifest.StatusProtocol -ne 5 -or
    $manifest.MaxSensitivityPercent -ne 3200) { throw 'Pacote antigo ou incompatível.' }
$expected = @("$stem.inf", "$stem.sys", "$stem.cat", 'GalaxyPenMapperTray.exe')
if (@($manifest.Files).Count -ne $expected.Count) { throw 'Manifesto incompleto.' }
foreach ($name in $expected) {
    $record = @($manifest.Files | Where-Object Name -eq $name)
    if ($record.Count -ne 1) { throw "Arquivo ausente ou duplicado no manifesto: $name" }
    $hash = (Get-FileHash -LiteralPath (Join-Path $package $name) -Algorithm SHA256).Hash
    if ($hash -ne $record[0].SHA256) { throw "Pacote alterado após validação: $name" }
}
foreach ($name in @("$stem.sys", "$stem.cat")) {
    $signature = Get-AuthenticodeSignature -LiteralPath (Join-Path $package $name)
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne 'CA50595A5BC03E62BF384B5DC39660D1E9210A9B') {
        throw "Assinatura inesperada: $name"
    }
}
$target = 'HID\WCOM016C&Col01\5&a6b5543&0&0000'
$oldTarget = 'HID\WCOM016C&Col04\5&a6b5543&0&0003'
$devices = @(Get-PnpDevice -InstanceId $target -PresentOnly -ErrorAction Stop)
if ($devices.Count -ne 1 -or $devices[0].Status -ne 'OK') { throw 'COL01 ausente ou com problema; instalação interrompida.' }
$stacks = foreach ($id in @($target, $oldTarget)) {
    $output = & pnputil.exe /enum-devices /instanceid $id /stack 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Não foi possível conferir a pilha: $id" }
    $output -join "`n"
}
$loadedMapper = ($stacks -join "`n") -match 'GalaxyPenPassThrough|GalaxyPenCol01Probe|GalaxyPenCol01Corrector'
$legacyLoaded = ($stacks -join "`n") -match 'GalaxyPenPassThrough'
$report = [pscustomobject]@{
    Variant = $Variant
    PackageIntegrity = 'OK'
    Col01Status = $devices[0].Status
    MapperAlreadyInStack = $loadedMapper
    MayInstall = (-not $loadedMapper)
    MayAttemptDeviceOnlyUpdate = ($loadedMapper -and -not $legacyLoaded)
    Action = $(if ($loadedMapper) { 'Concluir remoção do filtro anterior antes de instalar.' } else { 'Pilha livre; instalação ainda depende da política de assinatura de teste.' })
}
$report
if (-not $Install -and -not $Update) { return }
if ($legacyLoaded) { throw 'Filtro COL04 antigo ainda carregado, com remoção pendente. Nenhum novo driver foi instalado. A correção de descarregamento só vale para o novo filtro.' }
if ($loadedMapper -and -not $Update) { throw 'Filtro COL01 presente. Para atualização explícita, use -Update; nenhuma alteração feita.' }
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'A instalação deve ser executada como administrador.' }
$boot = (& bcdedit.exe /enum '{current}' 2>&1) -join "`n"
if ($LASTEXITCODE -ne 0 -or $boot -notmatch '(?im)^\s*testsigning\s+(Yes|Sim|On)\s*$') {
    throw 'TESTSIGNING não confirmado; nenhuma política de inicialização foi alterada.'
}
$runningTray = @(Get-Process -Name GalaxyPenMapperTray -ErrorAction SilentlyContinue)
if ($runningTray.Count) { throw 'Salve a captura e feche o Galaxy Pen Mapper antes de atualizar. Nenhum processo foi encerrado.' }
$oldPackages = @(Get-WindowsDriver -Online -ErrorAction Stop | Where-Object {
    [IO.Path]::GetFileName($_.OriginalFileName) -in @('galaxypencol01probe.inf', 'galaxypencol01corrector.inf') -and
    $_.ProviderName -eq 'Galaxy Pen Mapper (personal test)' -and $_.ClassName -eq 'Extension'
})
if ($oldPackages.Count -and -not $Update) { throw 'Há pacote COL01 anterior no Driver Store. Use -Update para evitar dois filtros na próxima anexação.' }
if ($Update) {
    # Resolve only our two COL01 extension INFs; never remove the Samsung base,
    # PenS2Helper, touch devices, the I2C parent, or an arbitrary oem*.inf.
    if ($loadedMapper -and -not $oldPackages.Count) { throw 'Filtro carregado sem pacote correspondente: remoção provavelmente pendente. Pare e confira a pilha.' }
    foreach ($old in $oldPackages) {
        if ($old.Driver -notmatch '^oem[0-9]+\.inf$') { throw 'Nome de pacote inesperado; remoção interrompida.' }
        & pnputil.exe /delete-driver $old.Driver /uninstall
        $removeCode = $LASTEXITCODE
        if ($removeCode -eq 3010) { throw 'Windows removeu o pacote, mas solicitou reinicialização. Atualização interrompida sem instalar outro filtro; o PC não será reiniciado automaticamente.' }
        if ($removeCode -ne 0) { throw "Remoção recusada ($removeCode). Não force o descarregamento." }
    }
    # A scoped PnP restart, not a computer reboot. Never use /force or /reboot.
    & pnputil.exe /restart-device $target
    if ($LASTEXITCODE -eq 3010) { throw 'Windows exige reinicialização do sistema. Nenhum novo filtro foi instalado.' }
    if ($LASTEXITCODE -ne 0) { throw 'O dispositivo recusou o reinício local. Atualização interrompida.' }
    $cleanStack = (& pnputil.exe /enum-devices /instanceid $target /stack 2>&1) -join "`n"
    if ($LASTEXITCODE -ne 0 -or $cleanStack -match 'GalaxyPenPassThrough|GalaxyPenCol01Probe|GalaxyPenCol01Corrector') {
        throw 'O filtro ainda não descarregou. Não é seguro instalar outro por cima.'
    }
    if ((Get-PnpDevice -InstanceId $target -PresentOnly -ErrorAction Stop).Status -ne 'OK') {
        throw 'O digitizador não voltou saudável; instalação interrompida.'
    }
}
$output = & pnputil.exe /add-driver (Join-Path $package "$stem.inf") /install 2>&1
$installCode = $LASTEXITCODE
$output | Set-Content -LiteralPath (Join-Path $repoRoot 'build-ninja\col01-install-last.txt') -Encoding UTF8
$output
if ($installCode -notin @(0, 3010)) { throw "Instalação retornou $installCode. Confira o relatório." }
if ($installCode -eq 3010) {
    throw 'Pacote instalado; Windows solicitou reinicialização. Não reiniciei automaticamente; validação pendente.'
}
$finalStack = (& pnputil.exe /enum-devices /instanceid $target /stack 2>&1) -join "`n"
if ($LASTEXITCODE -ne 0 -or $finalStack -notmatch [regex]::Escape($stem)) {
    throw 'Pacote armazenado, mas filtro não confirmado na pilha. Não é possível declarar a instalação operacional.'
}
if ((Get-PnpDevice -InstanceId $target -PresentOnly -ErrorAction Stop).Status -ne 'OK') {
    throw 'O dispositivo reportou problema após anexação. Não ative correção; confira o relatório e faça rollback do pacote do projeto.'
}
$finalStack
$service=Get-ItemProperty -LiteralPath ("HKLM:\SYSTEM\CurrentControlSet\Services\"+$stem)
$installedBinary=$service.ImagePath.Replace('\SystemRoot',$env:SystemRoot)
if((Get-FileHash -LiteralPath $installedBinary).Hash -ne (Get-FileHash -LiteralPath (Join-Path $package "$stem.sys")).Hash){
    throw 'Pilha presente, mas binário não corresponde ao novo pacote.'
}
Write-Host 'Filtro confirmado na pilha sem reiniciar o PC. Ainda é necessário comprovar leituras reais com a correção desligada.'
