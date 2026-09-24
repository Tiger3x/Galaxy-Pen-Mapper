# No PnP/SetupAPI calls: all effects are simulated in memory.
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '..\tools\DiagnosticDeployment.ps1')
function Require($ok,$label){if(-not $ok){throw "TEST FAILED: $label"}}
function Reset-Mock {
    $script:mock=@{State=[pscustomobject]@{Healthy=$true;Upper=@('PenS2Helper');Lower=@();Stack=$script:DiagOriginalStack.Clone()};
        Calls=[Collections.Generic.List[string]]::new();Journal=[Collections.Generic.List[string]]::new();RebootAt='';FailAt='';Services=$false;Corrector=$true}
}
function Effect([string]$name,[scriptblock]$change){
    $script:mock.Calls.Add($name)
    if($script:mock.FailAt -eq $name){throw 'Simulated failure'}
    & $change
    return ($script:mock.RebootAt -eq $name)
}
$ops=@{
    State={$script:mock.State}
    Journal={param($s)$script:mock.Journal.Add($s)}
    InstallServices={Effect 'InstallServices' {$script:mock.Services=$true}}
    RemoveCorrector={Effect 'RemoveCorrector' {$script:mock.Corrector=$false;$script:mock.State.Stack=$script:DiagCleanStack.Clone()}}
    Restart={Effect 'Restart' {
        $script:mock.State.Healthy=$true
        $script:mock.State.Stack=if($script:mock.State.Upper.Count -eq 3){$script:DiagTestStack.Clone()}elseif($script:mock.Corrector){$script:DiagOriginalStack.Clone()}else{$script:DiagCleanStack.Clone()}
    }}
    Attach={Effect 'Attach' {$script:mock.State.Upper=@('GalaxyPenDiagB','PenS2Helper','GalaxyPenDiagA')}}
    Detach={Effect 'Detach' {$script:mock.State.Upper=@('PenS2Helper')}}
    RemoveServices={Effect 'RemoveServices' {$script:mock.Services=$false}}
    InstallCorrector={Effect 'InstallCorrector' {$script:mock.Corrector=$true}}
}
Reset-Mock
Invoke-DiagInstall $ops
Require (Test-DiagList $mock.Calls @('InstallServices','RemoveCorrector','Restart','Attach','Restart')) 'Install order'
Require (Test-DiagList $mock.State.Stack $script:DiagTestStack) 'Diagnostic stack'
$mock.Calls.Clear()
Invoke-DiagRestore $ops
Require (Test-DiagList $mock.Calls @('Detach','Restart','RemoveServices','InstallCorrector','Restart')) 'Rollback order'
Require (Test-DiagList $mock.State.Stack $script:DiagOriginalStack) 'Original restored'
$mock.Calls.Clear();Invoke-DiagRestore $ops
Require (Test-DiagList $mock.Calls @('RemoveServices')) 'Idempotent restore'
foreach($point in @('InstallServices','RemoveCorrector','Restart','Attach')) {
    Reset-Mock;$mock.RebootAt=$point;$stopped=$false
    try{Invoke-DiagInstall $ops}catch{if($_ -notlike '*REBOOT_REQUIRED*'){throw};$stopped=$true}
    Require $stopped "Stop on reboot $point"
    Require ($mock.Calls[-1] -eq $point) "No action after reboot $point"
    $mock.RebootAt='';Invoke-DiagRestore $ops
    Require (Test-DiagList $mock.State.Stack $script:DiagOriginalStack) "Recover prefix $point"
}
foreach($point in @('InstallServices','RemoveCorrector','Attach')) {
    Reset-Mock;$mock.FailAt=$point;$stopped=$false
    try{Invoke-DiagInstall $ops}catch{$stopped=$true}
    Require $stopped "Failure stops $point"
    $mock.FailAt='';Invoke-DiagRestore $ops
    Require (Test-DiagList $mock.State.Stack $script:DiagOriginalStack) "Recover failure $point"
}
Reset-Mock;$mock.State.Upper=@('PenS2Helper','UnrelatedFilter')
try{Invoke-DiagInstall $ops;throw 'Accepted unknown filters'}catch{Require ($mock.Calls.Count -eq 0) 'Unknown filters untouched'}
try{Invoke-DiagRestore $ops;throw 'Accepted unknown filters'}catch{Require ($mock.Calls.Count -eq 0) 'Restore preserves unknown filters'}
Reset-Mock;$mock.State.Lower=@('UnknownLower')
try{Invoke-DiagInstall $ops;throw 'Accepted lower filter'}catch{Require ($mock.Calls.Count -eq 0) 'Unknown lower untouched'}
Reset-Mock;Invoke-DiagInstall $ops;$mock.Calls.Clear();$mock.RebootAt='Restart'
try{Invoke-DiagRestore $ops;throw 'Ignored reboot'}catch{Require ($_ -like '*REBOOT_REQUIRED*') 'Restore reboot signaled'}
Require (-not $mock.Calls.Contains('RemoveServices')) 'No removal before confirmed unload'
Reset-Mock;$badOps=$ops.Clone();$badOps.Restart={Effect 'Restart' {$script:mock.State.Stack=@('\Driver\Unexpected')}}
try{Invoke-DiagInstall $badOps;throw 'Accepted wrong stack'}catch{Require (-not $mock.Calls.Contains('Attach')) 'Wrong clean stack blocks attach'}
'Diagnostic deployment: install/restore, interrupted prefixes, reboot stops and unknown-stack guards passed (all OS effects mocked).'
