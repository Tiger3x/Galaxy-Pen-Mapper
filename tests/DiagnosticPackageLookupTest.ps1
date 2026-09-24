# Exercise the real lookup function with mocked OS access, without running main.
$ErrorActionPreference='Stop'
$file=Join-Path $PSScriptRoot '..\tools\manage-pressure-diagnostic.ps1'
$ast=[System.Management.Automation.Language.Parser]::ParseFile($file,[ref]$null,[ref]$null)
$function=$ast.Find({param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq 'Get-OwnedPackage'},$true)
. ([ScriptBlock]::Create($function.Extent.Text))
$script:packages=@([pscustomobject]@{OriginalFileName='C:\test\galaxypencol01corrector.inf';ProviderName='Galaxy Pen Mapper (personal test)';ClassName='Extension';Driver='oem188.inf'})
function Get-WindowsDriver {param([switch]$Online) $script:packages}
function Get-FileHash {param($LiteralPath)
    if($LiteralPath -ne 'expected.inf' -and $LiteralPath -ne (Join-Path $env:SystemRoot 'INF\oem188.inf')){throw "Unexpected hash target: $LiteralPath"}
    [pscustomobject]@{Hash='same'}
}
$found=Get-OwnedPackage 'galaxypencol01corrector.inf' 'Extension' 'expected.inf'
if($found -ne (Join-Path $env:SystemRoot 'INF\oem188.inf')){throw 'Published INF lost after regex validation.'}
$script:packages=@()
if($null -ne (Get-OwnedPackage 'galaxypencol01corrector.inf' 'Extension' 'expected.inf')){throw 'Absent package not null.'}
$script:packages=@([pscustomobject]@{OriginalFileName='galaxypencol01corrector.inf';ProviderName='Galaxy Pen Mapper (personal test)';ClassName='Extension';Driver='invalid.inf'})
$rejected=$false
try{Get-OwnedPackage 'galaxypencol01corrector.inf' 'Extension' 'expected.inf'}catch{$rejected=$_.Exception.Message -eq 'Nome publicado inválido.'}
if(-not $rejected){throw 'Invalid published name accepted.'}
'Package lookup regression passed; DISM and hashes mocked.'
