param(
    [Parameter(Mandatory=$true)][ValidateSet('Install','Restore')][string]$Action,
    [string]$PackageDirectory,
    [string]$RecoveryDirectory,
    [Parameter(Mandatory=$true)][string]$ResultPath
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $ResultPath){throw 'Resultado já existe; não sobrescrever.'}
$result=[ordered]@{Action=$Action;Completed=$false;Error=$null;Stack=$null;Utc=$null}
Start-Transcript -Path ($ResultPath+'.log') -NoClobber | Out-Null
try {
    $arguments=@{Action=$Action}
    if($PackageDirectory){$arguments.PackageDirectory=$PackageDirectory}
    if($RecoveryDirectory){$arguments.RecoveryDirectory=$RecoveryDirectory}
    & (Join-Path $PSScriptRoot 'manage-pressure-diagnostic.ps1') @arguments
    $result.Completed=$true
}catch{
    $result.Error=$_.Exception.Message
    Write-Host ($_ | Out-String)
}finally{
    try {$result.Stack=@((Get-PnpDeviceProperty -InstanceId 'HID\WCOM016C&Col01\5&a6b5543&0&0000' -KeyName DEVPKEY_Device_Stack).Data)}catch{}
    $result.Utc=[DateTime]::UtcNow.ToString('o')
    $result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $ResultPath -Encoding utf8
    Stop-Transcript | Out-Null
}
if(-not $result.Completed){exit 1}
