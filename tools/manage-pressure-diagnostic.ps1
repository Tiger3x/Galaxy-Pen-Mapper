param(
    [ValidateSet('Check','Install','Restore')][string]$Action='Check',
    [string]$PackageDirectory,
    [string]$RecoveryDirectory
)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'DiagnosticDeployment.ps1')
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target='HID\WCOM016C&Col01\5&a6b5543&0&0000'
$thumb='CA50595A5BC03E62BF384B5DC39660D1E9210A9B'
$journal=$null
function Get-DiagnosticState {
    $device=Get-PnpDevice -InstanceId $target -PresentOnly
    $collections=@(Get-PnpDevice -Class HIDClass -PresentOnly | Where-Object InstanceId -like 'HID\WCOM016C&Col01\*')
    if($collections.Count -ne 1){throw 'Mais de uma coleção alvo ou inventário inconsistente.'}
    $registry=Get-ItemProperty -LiteralPath ("HKLM:\SYSTEM\CurrentControlSet\Enum\"+$target)
    foreach($level in @('UpperFilterLevels','LowerFilterLevels','UpperFilterDefaultLevel','LowerFilterDefaultLevel')) {
        if($registry.PSObject.Properties[$level] -and $registry.$level){throw 'Níveis de filtro presentes: estratégia legacy não autorizada.'}
    }
    $properties=@{}
    foreach($key in @('DEVPKEY_Device_Stack','DEVPKEY_Device_UpperFilters','DEVPKEY_Device_LowerFilters','DEVPKEY_Device_Parent')) {
        $p=Get-PnpDeviceProperty -InstanceId $target -KeyName $key
        $properties[$key]=if($p.Type -eq 'Empty'){@()}else{@($p.Data)}
    }
    if (($properties['DEVPKEY_Device_Parent'] -join '') -ne 'ACPI\WCOM016C\1') { throw 'Pai do dispositivo inesperado.' }
    [pscustomobject]@{Healthy=($device.Status -eq 'OK');Stack=$properties['DEVPKEY_Device_Stack'];
        Upper=$properties['DEVPKEY_Device_UpperFilters'];Lower=$properties['DEVPKEY_Device_LowerFilters']}
}
function Assert-DiagnosticPackage([string]$Path,[bool]$Corrector) {
    $manifestName=if($Corrector){'build-manifest.json'}else{'manifest.json'}
    $m=Get-Content -Raw -LiteralPath (Join-Path $Path $manifestName) | ConvertFrom-Json
    if($Corrector) {
        if($m.Variant -ne 'Corrector' -or $m.DriverVersion -ne '0.3.1.0' -or $m.ConfigProtocol -ne 3 -or $m.StatusProtocol -ne 5) {throw 'Backup Corrector incompatível.'}
        $names=@('GalaxyPenCol01Corrector.sys','GalaxyPenCol01Corrector.cat','GalaxyPenCol01Corrector.inf','GalaxyPenMapperTray.exe')
    } else {
        if($m.Kind -ne 'PressureDiagnostic' -or $m.DriverVersion -ne '0.1.0.0' -or $m.Protocol -ne 1 -or -not $m.Signed) {throw 'Pacote diagnóstico incompatível.'}
        $names=@('GalaxyPenDiagA.sys','GalaxyPenDiagB.sys','GalaxyPenDiagnostic.inf','GalaxyPenDiagnostic.cat','GalaxyPenDiagnostic.exe')
    }
    if(@($m.Files).Count -ne $names.Count) {throw 'Manifesto incompleto.'}
    foreach($name in $names) {
        $record=@($m.Files | Where-Object Name -CEQ $name)
        if($record.Count -ne 1 -or (Get-FileHash -LiteralPath (Join-Path $Path $name) -Algorithm SHA256).Hash -ne $record[0].SHA256) {throw "Integridade inválida: $name"}
        if($name -match '\.(sys|cat)$') {
            $sig=Get-AuthenticodeSignature -LiteralPath (Join-Path $Path $name)
            if($sig.Status -ne 'Valid' -or $sig.SignerCertificate.Thumbprint -ne $thumb) {throw "Assinatura inválida: $name"}
        }
    }
}
function Get-OwnedPackage([string]$Leaf,[string]$Class,[string]$ExpectedInf) {
    $ownedPackages=@(Get-WindowsDriver -Online | Where-Object {
        [IO.Path]::GetFileName($_.OriginalFileName) -ieq $Leaf -and
        $_.ProviderName -eq 'Galaxy Pen Mapper (personal test)' -and $_.ClassName -eq $Class
    })
    if($ownedPackages.Count -gt 1) {throw 'Múltiplos pacotes do projeto; não escolher por tentativa.'}
    if($ownedPackages.Count) {
        if($ownedPackages[0].Driver -notmatch '^oem[0-9]+\.inf$') {throw 'Nome publicado inválido.'}
        $published=Join-Path $env:SystemRoot ('INF\'+$ownedPackages[0].Driver)
        if((Get-FileHash -LiteralPath $published).Hash -ne (Get-FileHash -LiteralPath $ExpectedInf).Hash) {throw 'Pacote instalado não corresponde ao backup/candidato.'}
        return $published
    }
    return $null
}
function Write-DiagnosticJournal([string]$Stage) {
    # Durable before every mutation. A interrupted/partial journal never authorizes new writes.
    $journal.Stage=$Stage;$journal.Utc=[DateTime]::UtcNow.ToString('o')
    $content=$journal | ConvertTo-Json -Depth 6
    $bytes=[Text.Encoding]::UTF8.GetBytes($content)
    $path=Join-Path $RecoveryDirectory 'journal.json'
    $pending=$path+'.next'
    $stream=[IO.FileStream]::new($pending,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try {$stream.Write($bytes,0,$bytes.Length);$stream.Flush($true)}finally{$stream.Dispose()}
    if(Test-Path -LiteralPath $path){[IO.File]::Replace($pending,$path,(Join-Path $RecoveryDirectory 'journal.previous.json'))}
    else{[IO.File]::Move($pending,$path)}
}
function Assert-OriginalBinary([string]$Backup) {
    $service=Get-ItemProperty -LiteralPath HKLM:\SYSTEM\CurrentControlSet\Services\GalaxyPenCol01Corrector
    $binary=$service.ImagePath.Replace('\SystemRoot',$env:SystemRoot)
    if((Get-FileHash -LiteralPath $binary).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Backup 'GalaxyPenCol01Corrector.sys')).Hash){throw 'Binário Corrector não corresponde ao backup 0.3.1.'}
}
function Restart-DiagnosticTarget {
    $output=& pnputil.exe /restart-device $target 2>&1
    $code=$LASTEXITCODE;$output | Out-Host
    if($code -eq 3010){return $true}
    if($code -ne 0){throw "Dispositivo recusou reinício local ($code)."}
    return $false
}
function Copy-VerifiedPackage([string]$Source,[string]$Destination,[bool]$Corrector) {
    Assert-DiagnosticPackage $Source $Corrector
    New-Item -ItemType Directory -Path $Destination | Out-Null
    $manifestName=if($Corrector){'build-manifest.json'}else{'manifest.json'}
    $m=Get-Content -Raw -LiteralPath (Join-Path $Source $manifestName) | ConvertFrom-Json
    foreach($name in @($m.Files.Name)+@($manifestName)) {Copy-Item -LiteralPath (Join-Path $Source $name) -Destination (Join-Path $Destination $name)}
    Assert-DiagnosticPackage $Destination $Corrector
}

$state=Get-DiagnosticState
if(-not ('GalaxyDiagnosticSetup' -as [type])){Add-Type -Path (Join-Path $PSScriptRoot 'DiagnosticSetupApi.cs')}
if(-not (Test-DiagList ([GalaxyDiagnosticSetup]::GetUpperFilters()) $state.Upper)) {
    throw 'SetupAPI e PnP discordam sobre UpperFilters; nenhuma alteração autorizada.'
}
if($Action -eq 'Check') {
    if(-not $PackageDirectory){throw 'Informe -PackageDirectory com o candidato assinado.'}
    Assert-DiagnosticPackage $PackageDirectory $false
    Assert-DiagStack $state $script:DiagOriginalStack
    Assert-DiagnosticPackage (Join-Path $repo 'build-ninja\driver\packages\col01-corrector') $true
    Assert-OriginalBinary (Join-Path $repo 'build-ninja\driver\packages\col01-corrector')
    $existingDiagnosticServices=@('GalaxyPenDiagA','GalaxyPenDiagB' | Where-Object {
        Test-Path -LiteralPath "HKLM:\SYSTEM\CurrentControlSet\Services\$_"
    })
    [pscustomobject]@{PackageIntegrity='OK';CurrentStack='Corrector > Samsung > mshidkmdf';
        TrayMustClose=(@(Get-Process GalaxyPenMapperTray -ErrorAction SilentlyContinue).Count -gt 0);
        DiagnosticServicesPresent=$existingDiagnosticServices;
        InstallBlocked=($existingDiagnosticServices.Count -gt 0);
        Action='Somente leitura. Instalação exige autorização explícita e elevação.'}
    return
}
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Ação exige administrador; nada alterado.'}
if([IntPtr]::Size -ne 8){throw 'Use PowerShell x64.'}
if(@(Get-Process GalaxyPenMapperTray,GalaxyPenDiagnostic -ErrorAction SilentlyContinue).Count){throw 'Salve dados e feche o app/coletor antes; nenhum processo foi encerrado.'}
$recoveryRoot=Join-Path $repo 'build-diagnostic\recovery'
if($Action -eq 'Install') {
    if($RecoveryDirectory){throw 'Install sempre cria backup novo; RecoveryDirectory é somente para Restore.'}
    if(-not $PackageDirectory){throw 'Pacote ausente.'}
    Assert-DiagStack $state $script:DiagOriginalStack
    if(-not (Test-DiagList $state.Upper @('PenS2Helper'))){throw 'Filtros iniciais inesperados.'}
    $boot=(& bcdedit.exe /enum '{current}' 2>&1)-join "`n"
    if($LASTEXITCODE -ne 0 -or $boot -notmatch '(?im)^\s*testsigning\s+(Yes|Sim|On)\s*$'){throw 'TESTSIGNING não confirmado; não foi alterado.'}
    $correctorSource=Join-Path $repo 'build-ninja\driver\packages\col01-corrector'
    Assert-DiagnosticPackage $PackageDirectory $false
    Assert-DiagnosticPackage $correctorSource $true
    Assert-OriginalBinary $correctorSource
    $oldInf=Get-OwnedPackage 'galaxypencol01corrector.inf' 'Extension' (Join-Path $correctorSource 'GalaxyPenCol01Corrector.inf')
    if(-not $oldInf){throw 'Pacote original não identificado.'}
    if(Get-OwnedPackage 'galaxypendiagnostic.inf' 'System' (Join-Path $PackageDirectory 'GalaxyPenDiagnostic.inf')){throw 'Diagnóstico já registrado. Usar Restore com seu backup.'}
    foreach($service in @('GalaxyPenDiagA','GalaxyPenDiagB')) {
        $servicePath="HKLM:\SYSTEM\CurrentControlSet\Services\$service"
        if(Test-Path -LiteralPath $servicePath){
            $existing=Get-ItemProperty -LiteralPath $servicePath
            if($existing.DeleteFlag -eq 1 -and $existing.Start -eq 4){
                throw "Serviço $service desativado e pendente de exclusão pelo Windows. Não repetir Install neste estado; concluir limpeza do Windows e executar Check novamente. Nada foi sobrescrito."
            }
            throw 'Serviço diagnóstico pré-existente; não sobrescrever.'
        }
    }
    $RecoveryDirectory=Join-Path $recoveryRoot ([Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $RecoveryDirectory | Out-Null
    Copy-VerifiedPackage $correctorSource (Join-Path $RecoveryDirectory 'corrector') $true
    Copy-VerifiedPackage $PackageDirectory (Join-Path $RecoveryDirectory 'diagnostic') $false
    $journal=[ordered]@{Version=1;Target=$target;OriginalUpper=@('PenS2Helper');OriginalPublishedInf=[IO.Path]::GetFileName($oldInf);Stage='prepared';Utc=''}
    Write-DiagnosticJournal 'prepared'
} else {
    if(-not $RecoveryDirectory){throw 'Informe o diretório de recuperação criado pela instalação.'}
    $RecoveryDirectory=(Resolve-Path -LiteralPath $RecoveryDirectory).Path
    $expectedRoot=[IO.Path]::GetFullPath($recoveryRoot).TrimEnd('\')+'\'
    if(-not $RecoveryDirectory.StartsWith($expectedRoot,[StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($RecoveryDirectory) -notmatch '^[a-f0-9]{32}$'){throw 'Diretório de recuperação fora do local esperado.'}
    try {$journal=Get-Content -Raw -LiteralPath (Join-Path $RecoveryDirectory 'journal.json') | ConvertFrom-Json -AsHashtable}
    catch {$journal=Get-Content -Raw -LiteralPath (Join-Path $RecoveryDirectory 'journal.previous.json') | ConvertFrom-Json -AsHashtable}
    if($journal.Version -ne 1 -or $journal.Target -cne $target -or -not (Test-DiagList $journal.OriginalUpper @('PenS2Helper'))){throw 'Registro de recuperação inválido.'}
    Assert-DiagnosticPackage (Join-Path $RecoveryDirectory 'corrector') $true
    Assert-DiagnosticPackage (Join-Path $RecoveryDirectory 'diagnostic') $false
}
$diagInf=Join-Path $RecoveryDirectory 'diagnostic\GalaxyPenDiagnostic.inf'
$correctorInf=Join-Path $RecoveryDirectory 'corrector\GalaxyPenCol01Corrector.inf'
$ops=@{
    State={Get-DiagnosticState}
    Journal={param($s) Write-DiagnosticJournal $s}
    InstallServices={ [GalaxyDiagnosticSetup]::Install($diagInf) }
    RemoveCorrector={
        $inf=Get-OwnedPackage 'galaxypencol01corrector.inf' 'Extension' $correctorInf
        if(-not $inf){throw 'Corrector sumiu antes da remoção; parar.'}
        [GalaxyDiagnosticSetup]::Uninstall($inf)
    }
    Restart={Restart-DiagnosticTarget}
    Attach={ [GalaxyDiagnosticSetup]::SetDiagnosticAttachment($true); $false }
    Detach={ [GalaxyDiagnosticSetup]::SetDiagnosticAttachment($false); $false }
    RemoveServices={
        $inf=Get-OwnedPackage 'galaxypendiagnostic.inf' 'System' $diagInf
        if($inf){[GalaxyDiagnosticSetup]::Uninstall($inf)}else{$false}
    }
    InstallCorrector={
        $output=& pnputil.exe /add-driver $correctorInf /install 2>&1
        $code=$LASTEXITCODE;$output | Out-Host
        if($code -eq 3010){$true}elseif($code -eq 0){$false}else{throw "Restauração retornou $code."}
    }
}
Write-Host "Recuperação preservada em: $RecoveryDirectory"
try {
    if($Action -eq 'Install'){Invoke-DiagInstall $ops}else{
        Invoke-DiagRestore $ops
        Assert-OriginalBinary (Join-Path $RecoveryDirectory 'corrector')
    }
    Write-Host "Operação concluída: $($journal.Stage). Nenhuma captura iniciada; PC não reiniciado."
}catch{
    Write-Warning "Operação interrompida. Não repita Install. Para retornar, use -Action Restore -RecoveryDirectory '$RecoveryDirectory'. Se o Windows pediu reboot, conclua-o antes."
    throw
}
