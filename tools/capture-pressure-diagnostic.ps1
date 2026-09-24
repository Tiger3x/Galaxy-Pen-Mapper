param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$package=Join-Path $repo 'build-diagnostic\packages\candidate-20260923-192218-665'
$collector=Join-Path $package 'GalaxyPenDiagnostic.exe'
$manifest=Get-Content -Raw -LiteralPath (Join-Path $package 'manifest.json') | ConvertFrom-Json
$expected=@($manifest.Files | Where-Object Name -CEQ 'GalaxyPenDiagnostic.exe')
if($expected.Count -ne 1 -or (Get-FileHash -LiteralPath $collector).Hash -ne $expected[0].SHA256){throw 'Coletor diferente do pacote verificado.'}
if(Test-Path -LiteralPath $OutputDirectory){throw 'Pasta já existe; não sobrescrever.'}
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Captura exige administrador.'}
& $collector --preflight
if($LASTEXITCODE -ne 0){throw 'Pilha não aprovada para captura.'}
Write-Host ''
Write-Host 'CAPTURA DE PRESSAO - 30 SEGUNDOS' -ForegroundColor Cyan
Write-Host 'Use uma area segura da tela, por exemplo uma tela em branco do Paint.'
Write-Host 'Faca toques e tracos leves a moderados, levantando a ponta entre eles.'
Write-Host 'Nao force a tela. Nao precisa usar os botoes. Pode aparecer como borracha.'
Read-Host 'Pressione ENTER quando estiver pronto' | Out-Null
Write-Host 'COMECE AGORA. A captura termina automaticamente em 30 segundos.' -ForegroundColor Green
try {
    & $collector --capture $OutputDirectory
    $code=$LASTEXITCODE
    if($code -ne 0){Write-Warning "Coletor retornou $code; o resultado sera verificado antes de tirar conclusoes."}
    else {Write-Host 'CAPTURA CONCLUIDA. Pode parar.' -ForegroundColor Green}
    Write-Host "Pasta: $OutputDirectory"
    if(Test-Path -LiteralPath (Join-Path $OutputDirectory 'result.txt')){Get-Content -LiteralPath (Join-Path $OutputDirectory 'result.txt')}
} finally {
    Read-Host 'Pressione ENTER para fechar esta janela' | Out-Null
}
