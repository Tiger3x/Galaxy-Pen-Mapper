# Read-only integration check; never calls Install, Uninstall or SetDiagnosticAttachment.
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot '..\tools\DiagnosticSetupApi.cs')
$field=[GalaxyDiagnosticSetup].GetField('UpperFiltersProperty',[Reflection.BindingFlags]'NonPublic,Static')
if($field.GetRawConstantValue() -ne 0x11){throw 'SPDRP_UPPERFILTERS must match SetupAPI.h (0x11).'}
$expected=@((Get-PnpDeviceProperty -InstanceId 'HID\WCOM016C&Col01\5&a6b5543&0&0000' -KeyName DEVPKEY_Device_UpperFilters).Data)
$actual=[GalaxyDiagnosticSetup]::GetUpperFilters()
if(($actual -join '|') -cne ($expected -join '|')){throw 'Native SetupAPI property differs from PnP property.'}
"Read-only SetupAPI integration passed: $($actual -join ', '). No device writes."
