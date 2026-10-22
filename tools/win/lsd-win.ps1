<#
 lsd-win.ps1 - Windows port of the `lsd` ADuC832 serial downloader.
 Same protocol and output messages as the Linux lsd flasher,
 implemented with .NET SerialPort so no compiler or extra driver tools are needed.

 Usage: powershell -File lsd-win.ps1 -Port COM3 [-Run] [-ConnectTimeout 8] file.hex
 Exit code: 0 on success, non-zero on any failure (unlike the Linux lsd).
#>
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [Parameter(Mandatory = $true, Position = 0)][string]$File,
    [switch]$Run,
    [switch]$NoModemControl,
    [int]$ConnectTimeout = 8,
    [int]$FastBaud = 230400
)

$ErrorActionPreference = 'Stop'
$script:total = 0
$script:sp = $null

function Fail([string]$msg, [int]$code = 1) {
    Write-Output " Failed: $msg"
    if ($script:sp -and $script:sp.IsOpen) { try { $script:sp.Close() } catch {} }
    Write-Output "Total bytes sent: $($script:total)."
    exit $code
}

function Read-Bytes([int]$count, [int]$timeoutMs) {
    $buf = New-Object byte[] $count
    $got = 0
    $sw = [Diagnostics.Stopwatch]::StartNew()
    while ($got -lt $count) {
        if ($script:sp.BytesToRead -gt 0) {
            $got += $script:sp.Read($buf, $got, $count - $got)
        } elseif ($sw.ElapsedMilliseconds -gt $timeoutMs) {
            return $null
        } else {
            Start-Sleep -Milliseconds 2
        }
    }
    return , $buf
}

function Send([byte[]]$bytes) {
    $script:sp.Write($bytes, 0, $bytes.Length)
    $script:total += $bytes.Length
}

function Checksum([byte[]]$bytes) {
    $sum = 0
    foreach ($b in $bytes) { $sum += $b }
    return [byte]((0 - $sum) -band 0xFF)
}

function Expect-Ack([int]$timeoutMs, [string]$what) {
    $r = Read-Bytes 1 $timeoutMs
    if ($null -eq $r) { Fail "No answer from loader during $what." 2 }
    if ($r[0] -ne 0x06) { Fail 'NAK.' 3 }
}

function Send-Command([char]$cmd, [byte[]]$data, [int]$timeoutMs, [string]$what) {
    $body = New-Object System.Collections.Generic.List[byte]
    $body.Add([byte]($data.Length + 1)); $body.Add([byte][int]$cmd)
    foreach ($d in $data) { $body.Add($d) }
    $pkt = New-Object System.Collections.Generic.List[byte]
    $pkt.Add(0x07); $pkt.Add(0x0E)
    $pkt.AddRange($body)
    $pkt.Add((Checksum $body.ToArray()))
    Send $pkt.ToArray()
    Expect-Ack $timeoutMs $what
}

# --- read the hex file: data bytes are packed sequentially (the extension normalises the file first)
if (-not (Test-Path -LiteralPath $File)) { Write-Output "Error: Could not open file '$File'."; exit 4 }
$image = New-Object System.Collections.Generic.List[byte]
$lineNo = 0
foreach ($line in [IO.File]::ReadAllLines($File)) {
    $lineNo++
    $line = $line.Trim()
    if ($line.Length -eq 0) { continue }
    if ($line[0] -ne ':' -or ($line.Length % 2) -ne 1) { Write-Output "Error: Invalid hex record on line $lineNo."; exit 4 }
    $rec = New-Object byte[] (($line.Length - 1) / 2)
    for ($i = 0; $i -lt $rec.Length; $i++) { $rec[$i] = [Convert]::ToByte($line.Substring(1 + 2 * $i, 2), 16) }
    $sum = 0; foreach ($b in $rec) { $sum += $b }
    if (($sum -band 0xFF) -ne 0) { Write-Output "Error: Checksum error on line $lineNo."; exit 4 }
    $len = $rec[0]; $type = $rec[3]
    if ($type -eq 0) { for ($i = 0; $i -lt $len; $i++) { $image.Add($rec[4 + $i]) } }
    elseif ($type -eq 1) { break }
}
if ($image.Count -eq 0) { Write-Output 'Error: Hex file contains no data.'; exit 4 }
while (($image.Count % 256) -ne 0) { $image.Add(0xFF) }
$pages = $image.Count / 256

try {
    $script:sp = New-Object System.IO.Ports.SerialPort $Port, 9600, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
    if (-not $NoModemControl) {
        $script:sp.DtrEnable = $true
        $script:sp.RtsEnable = $true
    }
    $script:sp.ReadBufferSize = 4096
    $script:sp.Open()
} catch {
    Write-Output "Could not open '$Port', make sure device is present. ($($_.Exception.Message))"
    exit 5
}

Write-Output 'Connect to loader.'
$script:sp.DiscardInBuffer()
$reply = $null
$sw = [Diagnostics.Stopwatch]::StartNew()
while ($null -eq $reply -and $sw.Elapsed.TotalSeconds -lt $ConnectTimeout) {
    Send ([byte[]](0x21, 0x5A, 0x00, 0xA6))
    $reply = Read-Bytes 25 1000
    if ($null -eq $reply) { $script:sp.DiscardInBuffer() }
}
if ($null -eq $reply) { Fail 'No answer from loader. Put the board in download mode and try again.' 2 }
$text = [Text.Encoding]::ASCII.GetString($reply, 0, 7)
$sum = 0; for ($i = 0; $i -lt 24; $i++) { $sum += $reply[$i] }
if ($text -ne 'ADI 832' -or $reply[24] -ne [byte]((0 - $sum) -band 0xFF)) { Fail 'Unexpected answer from loader (not an ADuC832?).' 2 }
Write-Output ' Loader connected.'

Send-Command 'B' ([byte[]](0x81, 0x09)) 5000 'baud rate change'
$script:sp.BaudRate = $FastBaud
Start-Sleep -Milliseconds 50
$script:sp.DiscardInBuffer()

Send-Command 'A' ([byte[]]@()) 30000 'erase'

for ($p = 0; $p -lt $pages; $p++) {
    $body = New-Object byte[] 257
    $body[0] = [byte]$p
    $image.CopyTo($p * 256, $body, 1, 256)
    $pkt = New-Object byte[] 260
    $pkt[0] = 0x07; $pkt[1] = 0x51
    [Array]::Copy($body, 0, $pkt, 2, 257)
    $pkt[259] = Checksum $body
    Send $pkt
    Expect-Ack 5000 "page $p"
}

Send-Command 'B' ([byte[]](0x85, 0x2D)) 5000 'baud rate restore'
$script:sp.BaudRate = 9600

if ($Run) { Send-Command 'U' ([byte[]](0x00, 0x00, 0x00)) 5000 'run' }

$script:sp.Close()
Write-Output "Total bytes sent: $($script:total)."
exit 0
