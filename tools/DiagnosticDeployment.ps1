# Pure orchestration. All effects are injected, so failure paths can be tested
# without importing SetupAPI, accessing PnP or changing the registry.
$script:DiagOriginalStack = @('\Driver\GalaxyPenCol01Corrector','\Driver\PenS2Helper','\Driver\mshidkmdf')
$script:DiagCleanStack = @('\Driver\PenS2Helper','\Driver\mshidkmdf')
$script:DiagTestStack = @('\Driver\GalaxyPenDiagA','\Driver\PenS2Helper','\Driver\GalaxyPenDiagB','\Driver\mshidkmdf')
function Test-DiagList($Actual, $Expected) { return (($Actual -join '|') -ceq ($Expected -join '|')) }
function Assert-DiagFilters($State) {
    if (@($State.Lower).Count -or (-not (Test-DiagList $State.Upper @('PenS2Helper')) -and
        -not (Test-DiagList $State.Upper @('GalaxyPenDiagB','PenS2Helper','GalaxyPenDiagA')))) {
        throw 'Filtros inesperados: não sobrescrever a configuração.'
    }
}
function Assert-DiagStack($State, $Expected) {
    Assert-DiagFilters $State
    if (-not $State.Healthy -or -not (Test-DiagList $State.Stack $Expected)) {
        throw 'Pilha real/saúde diferente da esperada; captura bloqueada.'
    }
}
function Invoke-DiagStep($Ops, [string]$Name) {
    & $Ops.Journal "before:$Name"
    if ([bool](& $Ops[$Name])) {
        & $Ops.Journal "reboot-required:$Name"
        throw 'REBOOT_REQUIRED: Windows pediu reinício. Nenhum passo posterior foi executado; não reiniciei o PC.'
    }
    & $Ops.Journal "after:$Name"
}
function Invoke-DiagInstall($Ops) {
    $state = & $Ops.State
    Assert-DiagStack $state $script:DiagOriginalStack
    if (-not (Test-DiagList $state.Upper @('PenS2Helper'))) { throw 'UpperFilters inicial inesperado.' }
    Invoke-DiagStep $Ops 'InstallServices'
    Assert-DiagStack (& $Ops.State) $script:DiagOriginalStack
    Invoke-DiagStep $Ops 'RemoveCorrector'
    Invoke-DiagStep $Ops 'Restart'
    Assert-DiagStack (& $Ops.State) $script:DiagCleanStack
    Invoke-DiagStep $Ops 'Attach'
    Invoke-DiagStep $Ops 'Restart'
    $state = & $Ops.State
    Assert-DiagStack $state $script:DiagTestStack
    if (-not (Test-DiagList $state.Upper @('GalaxyPenDiagB','PenS2Helper','GalaxyPenDiagA'))) { throw 'Registro não confirmou anexação.' }
    & $Ops.Journal 'installed-awaiting-physical-validation'
}
function Invoke-DiagRestore($Ops) {
    $state = & $Ops.State
    Assert-DiagFilters $state
    if (Test-DiagList $state.Upper @('GalaxyPenDiagB','PenS2Helper','GalaxyPenDiagA')) {
        Invoke-DiagStep $Ops 'Detach'
        Invoke-DiagStep $Ops 'Restart'
    } elseif (-not $state.Healthy -or -not ((Test-DiagList $state.Stack $script:DiagCleanStack) -or
                (Test-DiagList $state.Stack $script:DiagOriginalStack))) {
        Invoke-DiagStep $Ops 'Restart'
    }
    $state = & $Ops.State
    # Never delete a driver package while one of its observers is still attached.
    if (-not $state.Healthy -or -not ((Test-DiagList $state.Stack $script:DiagCleanStack) -or
                (Test-DiagList $state.Stack $script:DiagOriginalStack))) { throw 'Descarga não confirmada. Pacote diagnóstico preservado.' }
    Invoke-DiagStep $Ops 'RemoveServices'
    $state = & $Ops.State
    if (-not (Test-DiagList $state.Stack $script:DiagOriginalStack)) {
        Invoke-DiagStep $Ops 'InstallCorrector'
        Invoke-DiagStep $Ops 'Restart'
    }
    Assert-DiagStack (& $Ops.State) $script:DiagOriginalStack
    & $Ops.Journal 'restored'
}
