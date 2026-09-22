param(
    [Parameter(Mandatory = $false)]
    [string]$CaptureDirectory = (Join-Path $PSScriptRoot '..\build-ninja\captures')
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $CaptureDirectory -PathType Container)) {
    throw "Capture directory not found: $CaptureDirectory"
}

Get-ChildItem -LiteralPath $CaptureDirectory -Filter '*.csv' |
    Sort-Object LastWriteTime |
    ForEach-Object {
        $rows = @(Import-Csv -LiteralPath $_.FullName)
        $sessionStart = $rows | Where-Object event -eq 'SESSION_START' | Select-Object -First 1
        $session = $sessionStart.session
        $primaryRaw = @(
            $rows |
                Where-Object {
                    $_.event -eq 'RAW_HID' -and
                    $_.usage_page -eq '13' -and
                    $_.usage -in @('1', '2')
                }
        )
        $primaryReport = @(
            $primaryRaw |
                ForEach-Object { ($_.raw_hex -split ' ')[0] } |
                Group-Object |
                Sort-Object Count -Descending |
                Select-Object -First 1
        ).Name

        if (-not $primaryReport) {
            Write-Warning "No primary Digitizer report found for session '$session'; skipping $($_.Name)."
            return
        }

        $pairs = [System.Collections.Generic.List[object]]::new()

        for ($index = 0; $index -lt $rows.Count; $index++) {
            $pointer = $rows[$index]
            if ($pointer.event -notlike 'POINTER_*') { continue }

            for ($rawIndex = $index + 1; $rawIndex -lt [Math]::Min($rows.Count, $index + 10); $rawIndex++) {
                $candidate = $rows[$rawIndex]
                if ($candidate.event -like 'POINTER_*') { break }
                if ($candidate.event -ne 'RAW_HID') { continue }

                $bytes = @($candidate.raw_hex -split ' ')
                if ($bytes[0] -ne $primaryReport) { continue }

                $rawPressure = [Convert]::ToInt32($bytes[6], 16) +
                    (256 * [Convert]::ToInt32($bytes[7], 16))

                $pairs.Add([PSCustomObject]@{
                    Event = $pointer.event
                    InContact = (([int]$pointer.pointer_flags -band 0x04) -ne 0)
                    PenFlags = [int]$pointer.pen_flags
                    PointerPressure = [int]$pointer.pressure
                    RawFlags = $bytes[1]
                    RawPressure = $rawPressure
                })
                break
            }
        }

        $contact = @($pairs | Where-Object InContact)
        $ratios = @(
            $contact |
                Where-Object PointerPressure -gt 0 |
                ForEach-Object { $_.RawPressure / $_.PointerPressure } |
                Sort-Object
        )
        $medianRatio = if ($ratios.Count -gt 0) { $ratios[[int]($ratios.Count / 2)] } else { $null }
        $pointerRows = @($rows | Where-Object event -like 'POINTER_*')
        $systemEvents = @(
            $rows |
                Where-Object { $_.event -like 'RAW_MOUSE' -or $_.event -like 'RAW_KEYBOARD' -or $_.event -like 'WINDOW_*' } |
                Group-Object event |
                Sort-Object Name |
                ForEach-Object { "$($_.Name):$($_.Count)" }
        )

        [PSCustomObject]@{
            File = $_.Name
            Session = $session
            ReportId = $primaryReport
            MatchedEvents = $pairs.Count
            ContactEvents = $contact.Count
            PointerDown = @($pointerRows | Where-Object event -eq 'POINTER_DOWN').Count
            PointerUp = @($pointerRows | Where-Object event -eq 'POINTER_UP').Count
            PointerEnter = @($pointerRows | Where-Object event -eq 'POINTER_ENTER').Count
            PointerLeave = @($pointerRows | Where-Object event -eq 'POINTER_LEAVE').Count
            PointerPressureMin = ($contact.PointerPressure | Measure-Object -Minimum).Minimum
            PointerPressureMax = ($contact.PointerPressure | Measure-Object -Maximum).Maximum
            RawPressureMin = ($contact.RawPressure | Measure-Object -Minimum).Minimum
            RawPressureMax = ($contact.RawPressure | Measure-Object -Maximum).Maximum
            MedianRawPerPointer = if ($null -eq $medianRatio) { $null } else { [Math]::Round($medianRatio, 4) }
            RawFlagCounts = (($pairs | Group-Object RawFlags | Sort-Object Name | ForEach-Object { "$($_.Name):$($_.Count)" }) -join ', ')
            PenFlagCounts = (($pairs | Group-Object PenFlags | Sort-Object Name | ForEach-Object { "$($_.Name):$($_.Count)" }) -join ', ')
            SystemEvents = $systemEvents -join ', '
        }
    } |
    Format-Table -AutoSize
