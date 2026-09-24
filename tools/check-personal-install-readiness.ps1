param(
    [string]$CertificateThumbprint = 'CA50595A5BC03E62BF384B5DC39660D1E9210A9B'
)

$ErrorActionPreference = 'Continue'
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

$secureBoot = 'indisponível'
try {
    $secureBoot = [string](Confirm-SecureBootUEFI -ErrorAction Stop)
} catch {
    try {
        $registryValue = Get-ItemPropertyValue -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\SecureBoot\State' `
            -Name UEFISecureBootEnabled -ErrorAction Stop
        if ($registryValue -eq 0) { $secureBoot = 'False (confirmado pela leitura somente de registro)' }
        elseif ($registryValue -eq 1) { $secureBoot = 'True (confirmado pela leitura somente de registro)' }
        else { $secureBoot = "valor de registro não esperado: $registryValue" }
    } catch {
        $secureBoot = "indisponível ($($_.Exception.Message))"
    }
}

$bitLocker = 'indisponível'
try {
    $volume = Get-BitLockerVolume -MountPoint 'C:' -ErrorAction Stop
    $bitLocker = "proteção=$($volume.ProtectionStatus); estado=$($volume.VolumeStatus)"
} catch {
    $bitLocker = "indisponível ($($_.Exception.Message))"
}

$boot = @()
try { $boot = @(& bcdedit.exe /enum '{current}' 2>&1) } catch { }
$testSigning = 'não confirmado'
$bootText = $boot -join "`n"
if ($bootText -match '(?im)^\s*testsigning\s+(Yes|Sim|On)\s*$') { $testSigning = 'ativado' }
elseif ($bootText -match '(?im)^\s*testsigning\s+(No|Não|Off)\s*$') { $testSigning = 'desativado' }
elseif ($bootText -match '(?im)^\s*testsigning\s+') { $testSigning = 'configurado; confira a saída do BCDEdit' }

$localRoot = Get-ChildItem Cert:\LocalMachine\Root -ErrorAction SilentlyContinue |
    Where-Object Thumbprint -eq $CertificateThumbprint
$localPublisher = Get-ChildItem Cert:\LocalMachine\TrustedPublisher -ErrorAction SilentlyContinue |
    Where-Object Thumbprint -eq $CertificateThumbprint
$personalCert = Get-ChildItem Cert:\CurrentUser\My -ErrorAction SilentlyContinue |
    Where-Object Thumbprint -eq $CertificateThumbprint

$pen = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
    Where-Object InstanceId -like 'HID\WCOM016C&Col04*' | Select-Object -First 1
$filterState = 'caneta WCOM COL04 não localizada'
if ($pen) {
    $upperFilters = @()
    try {
        $property = Get-PnpDeviceProperty -InstanceId $pen.InstanceId -KeyName 'DEVPKEY_Device_UpperFilters' -ErrorAction Stop
        $upperFilters = @($property.Data)
    } catch { }
    if ($upperFilters -contains 'GalaxyPenPassThrough') {
        $filterState = 'filtro GalaxyPenPassThrough já aparece em UpperFilters'
    } else {
        $filterState = 'caneta presente; filtro GalaxyPenPassThrough não aparece em UpperFilters'
    }
}

[pscustomobject]@{ Check = 'Sessão administrativa'; Result = $(if ($isAdmin) { 'sim' } else { 'não — execute numa janela elevada para verificar todos os estados' }) }
[pscustomobject]@{ Check = 'Secure Boot UEFI'; Result = $secureBoot }
[pscustomobject]@{ Check = 'BitLocker C:'; Result = $bitLocker }
[pscustomobject]@{ Check = 'Windows TESTSIGNING'; Result = $testSigning }
[pscustomobject]@{ Check = 'Certificado pessoal com chave privada'; Result = $(if ($personalCert -and $personalCert.HasPrivateKey) { 'presente no CurrentUser\My' } else { 'ausente no CurrentUser\My' }) }
[pscustomobject]@{ Check = 'Certificado confiável no computador'; Result = "Root=$([bool]$localRoot); TrustedPublisher=$([bool]$localPublisher)" }
[pscustomobject]@{ Check = 'Coleção de caneta'; Result = $filterState }

Write-Host "`nNenhuma configuração foi alterada por este script. Não compartilhe chaves de recuperação do BitLocker."
