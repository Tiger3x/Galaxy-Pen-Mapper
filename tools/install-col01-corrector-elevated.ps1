param([Parameter(Mandatory = $true)][string]$ResultPath, [switch]$Update, [string]$PackageDirectory)

# Elevated entry point with a durable result; never reboots or enables correction.
$ErrorActionPreference = 'Stop'
if(Test-Path -LiteralPath $ResultPath){throw 'Resultado já existe; não sobrescrever.'}
$result = [ordered]@{ Completed = $false; Error = $null; Stack = $null; TimestampUtc = $null }
Start-Transcript -Path ($ResultPath + '.log') -Force | Out-Null
try {
    $options=@{Variant='Corrector'}
    if($PackageDirectory){$options.PackageDirectory=$PackageDirectory}
    if ($Update) { $options.Update=$true } else { $options.Install=$true }
    & (Join-Path $PSScriptRoot 'install-col01-driver.ps1') @options
    $result.Stack = (& pnputil.exe /enum-devices /instanceid 'HID\WCOM016C&Col01\5&a6b5543&0&0000' /stack 2>&1) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'Não foi possível registrar a pilha após instalação.' }
    $result.Completed = $true
} catch {
    $result.Error = $_.Exception.Message
    Write-Host $result.Error
} finally {
    $result.TimestampUtc = [DateTime]::UtcNow.ToString('o')
    $result | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $ResultPath -Encoding UTF8
    Stop-Transcript | Out-Null
}
if (-not $result.Completed) { exit 1 }
