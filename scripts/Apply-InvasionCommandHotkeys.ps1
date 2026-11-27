param(
    [Parameter(Mandatory = $true)]
    [string] $ModDirectory
)

$ErrorActionPreference = 'Stop'
$mod = (Resolve-Path -LiteralPath $ModDirectory).Path
$wndEncoding = [Text.Encoding]::GetEncoding(28591)
$updates = [ordered]@{}
$key = 'GUI:ShowCommandHotkeys'

# Keep the original bytes of the WND files outside the edited fields.
$controlBarPath = 'Window\ControlBar.wnd'
$controlBar = $wndEncoding.GetString([IO.File]::ReadAllBytes((Join-Path $mod $controlBarPath)))
$fontPattern = '(?m)^(?<prefix>[ \t]*NAME = "ControlBar\.wnd:ButtonCommand(?<slot>0[1-9]|1[0-8])";\r?\n(?:[^\r\n]*\r?\n){6}[ \t]*FONT = NAME: "Arial", SIZE: )\d+, BOLD: [01];'
$fonts = [regex]::Matches($controlBar, $fontPattern)
$slots = @($fonts | ForEach-Object { $_.Groups['slot'].Value } | Sort-Object -Unique)
if ($fonts.Count -ne 18 -or $slots.Count -ne 18) {
    throw 'Could not find one Arial font for each of the 18 command slots.'
}
$newControlBar = [regex]::Replace($controlBar, $fontPattern, '${prefix}16, BOLD: 1;')
if ($newControlBar -cne $controlBar) {
    $updates[$controlBarPath] = $wndEncoding.GetBytes($newControlBar)
}

$optionsPath = 'Window\Menus\OptionsMenu.wnd'
$options = $wndEncoding.GetString([IO.File]::ReadAllBytes((Join-Path $mod $optionsPath)))
$checkboxName = 'OptionsMenu.wnd:CheckShowCommandHotkeys'
if (!$options.Contains('NAME = "' + $checkboxName + '";')) {
    $blockPattern = '(?ms)^ {6}CHILD\r?\n {6}WINDOW\r?\n.*?^ {6}END\r?\n'
    $checkboxes = @([regex]::Matches($options, $blockPattern) | Where-Object {
        $_.Value.Contains('NAME = "OptionsMenu.wnd:CheckBoxSaveCamera";')
    })
    if ($checkboxes.Count -ne 1) {
        throw 'Could not find the replay camera checkbox in the Invasion options layout.'
    }
    $template = $checkboxes[0]
    $checkbox = $template.Value.Replace('OptionsMenu.wnd:CheckBoxSaveCamera', $checkboxName).Replace('GUI:SaveCamera', $key)
    $checkbox = $checkbox -replace 'UPPERLEFT: \d+ \d+,', 'UPPERLEFT: 550 145,'
    $checkbox = $checkbox -replace 'BOTTOMRIGHT: \d+ \d+,', 'BOTTOMRIGHT: 738 169,'
    $updates[$optionsPath] = $wndEncoding.GetBytes($options.Insert($template.Index, $checkbox))
}

function Add-CsfLabel([byte[]] $original, [string] $label, [string] $text) {
    $stream = [IO.MemoryStream]::new()
    $stream.Write($original, 0, $original.Length)
    $stream.Position = 0
    $reader = [IO.BinaryReader]::new($stream)
    try {
        if ($reader.ReadInt32() -ne 0x43534620) { throw 'Invalid CSF header.' }
        $stream.Position = 8
        $labels = $reader.ReadInt32()
        $strings = $reader.ReadInt32()
        $stream.Position = 24
        $stringCount = 0
        $hasLabel = $false
        for ($i = 0; $i -lt $labels; $i++) {
            if ($reader.ReadInt32() -ne 0x4C424C20) { throw 'Invalid CSF label record.' }
            $count = $reader.ReadInt32()
            $name = [Text.Encoding]::ASCII.GetString($reader.ReadBytes($reader.ReadInt32()))
            $hasLabel = $hasLabel -or ($name -eq $label)
            $stringCount += $count
            for ($j = 0; $j -lt $count; $j++) {
                $kind = $reader.ReadInt32()
                if ($kind -ne 0x53545220 -and $kind -ne 0x53545257) { throw 'Invalid CSF string record.' }
                $length = $reader.ReadInt32()
                $stream.Position += 2L * $length
                if ($kind -eq 0x53545257) {
                    $extraLength = $reader.ReadInt32()
                    $stream.Position += $extraLength
                }
            }
        }
        if ($stream.Position -ne $original.Length -or $strings -ne $stringCount) { throw 'Invalid CSF record counts.' }
        if ($hasLabel) { return ,$original }

        $labelBytes = [Text.Encoding]::ASCII.GetBytes($label)
        $textBytes = [Text.Encoding]::Unicode.GetBytes($text)
        for ($i = 0; $i -lt $textBytes.Length; $i++) {
            $textBytes[$i] = $textBytes[$i] -bxor 255
        }

        $writer = [IO.BinaryWriter]::new($stream)
        try {
            $writer.Write([int]0x4C424C20)
            $writer.Write([int]1)
            $writer.Write([int]$labelBytes.Length)
            $writer.Write($labelBytes)
            $writer.Write([int]0x53545220)
            $writer.Write([int]$text.Length)
            $writer.Write($textBytes)
            $stream.Position = 8
            $writer.Write([int]($labels + 1))
            $writer.Write([int]($strings + 1))
            return ,$stream.ToArray()
        } finally {
            $writer.Dispose()
        }
    } finally {
        $reader.Dispose()
    }
}

$translations = [ordered]@{
    'Data\English\generals.csf' = 'Show Hotkeys'
    'Data\English\generals_ru.csf' = 'Показывать хоткеи'
}
foreach ($path in $translations.Keys) {
    $original = [IO.File]::ReadAllBytes((Join-Path $mod $path))
    $updated = Add-CsfLabel $original $key $translations[$path]
    if ($updated.Length -ne $original.Length) {
        $updates[$path] = $updated
    }
}
if ($updates.Count -eq 0) {
    Write-Output 'Command hotkey assets are already installed.'
    return
}

$backupName = 'CommandSetHotkeyAssets-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffZ')
$backup = Join-Path (Split-Path -Parent $mod) ('Backups\' + $backupName)
$modName = Split-Path -Leaf $mod
New-Item -ItemType Directory -Path $backup | Out-Null
$manifest = @('Existing files backed up with SHA-256:')
foreach ($path in $updates.Keys) {
    $source = Join-Path $mod $path
    $saved = Join-Path (Join-Path $backup $modName) $path
    New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $saved
    $hash = (Get-FileHash -LiteralPath $saved).Hash
    if ($hash -ne (Get-FileHash -LiteralPath $source).Hash) { throw "Backup mismatch for $path" }
    $manifest += "$hash  $modName\$path"
}
$manifest += 'New mod files to remove on rollback: none'
Set-Content -LiteralPath (Join-Path $backup 'MANIFEST.txt') -Encoding UTF8 -Value $manifest

$written = @()
try {
    foreach ($path in $updates.Keys) {
        $written += $path
        [IO.File]::WriteAllBytes((Join-Path $mod $path), $updates[$path])
    }
} catch {
    foreach ($path in $written) {
        Copy-Item -LiteralPath (Join-Path (Join-Path $backup $modName) $path) -Destination (Join-Path $mod $path) -Force
    }
    throw
}
Write-Output "Command hotkey assets installed. Backups saved in $backup."
