# Uses real package integrity checks but mocks all PnP access: never touches devices.
param([string]$PackageDirectory)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$global:GalaxyInstallerTestLegacy = $true
$global:GalaxyInstallerTestCalls = [Collections.Generic.List[string]]::new()
function Get-PnpDevice {
    [CmdletBinding()]param([string]$InstanceId, [switch]$PresentOnly)
    [pscustomobject]@{ Status = 'OK'; InstanceId = $InstanceId }
}
function pnputil.exe {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments)
    $global:GalaxyInstallerTestCalls.Add(($Arguments -join ' '))
    if ($Arguments[0] -ne '/enum-devices') { throw 'TEST FAILED: attempted device mutation.' }
    $global:LASTEXITCODE = 0
    if ($global:GalaxyInstallerTestLegacy) { 'GalaxyPenPassThrough' } else { 'PenS2Helper mshidkmdf' }
}
$installer = Join-Path $repo 'tools\install-col01-driver.ps1'
$packageOptions=@{}
if($PackageDirectory){$packageOptions.PackageDirectory=$PackageDirectory}
$check = & $installer -Variant Corrector @packageOptions
if ($check.MayInstall -or $check.MayAttemptDeviceOnlyUpdate) { throw 'Legacy stack not blocked.' }
foreach ($action in @('Install', 'Update')) {
    $options = @{ Variant = 'Corrector'; $action = $true }
    $blocked = $false
    try { & $installer @options @packageOptions | Out-Null }
    catch {
        if ($_.Exception.Message -notlike '*COL04 antigo*') { throw }
        $blocked = $true
    }
    if (-not $blocked) { throw 'Legacy mutation accepted.' }
}
$global:GalaxyInstallerTestLegacy = $false
$check = & $installer -Variant Corrector @packageOptions
if (-not $check.MayInstall) { throw 'Clean stack incorrectly rejected.' }
if (@($global:GalaxyInstallerTestCalls | Where-Object { $_ -notlike '/enum-devices *' }).Count) { throw 'Unexpected mutating command.' }
'Installer safety: check-only and legacy Install/Update guards passed; PnP access mocked.'
