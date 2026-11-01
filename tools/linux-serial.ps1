param(
    [Parameter(Mandatory=$true)][string]$CommandFile,
    [string]$Label='linux-diagnostics',
    [int]$TimeoutSeconds=90,
    [string]$CaptureBase64=''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$t = Get-Content "$root\private\device-target.json" -Raw | ConvertFrom-Json
$mutex = New-Object System.Threading.Mutex($false, "Local\cupid-device-$($t.serial)")
if (-not $mutex.WaitOne(0)) { throw 'Another operator holds the designated device.' }
$log = "$root\private\logs\$Label-$(Get-Date -Format yyyyMMdd-HHmmss).txt"
$sp = $null
try {
    $ports = @(Get-CimInstance Win32_SerialPort | Where-Object { $_.PNPDeviceID -match 'VID_0525&PID_A4A7|VID_1D6B&PID_0104' })
    if ($ports.Count -ne 1) { throw 'Exactly one known Linux ACM debug interface is required.' }
    $sp = New-Object System.IO.Ports.SerialPort($ports[0].DeviceID,115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
    $sp.ReadTimeout = 500
    $sp.WriteTimeout = 10000
    $sp.ReadBufferSize = 65536
    $sp.DtrEnable = $true
    $sp.Open()
    $transcript = [System.Text.StringBuilder]::new()
    [void]$transcript.AppendLine("Target=$($t.serial); Port=$($ports[0].DeviceID); HostTime=$(Get-Date -Format o)")
    function ReadFor([double]$Seconds) {
        $buf = [System.Text.StringBuilder]::new()
        $stop = (Get-Date).AddSeconds($Seconds)
        do {
            if ($sp.BytesToRead -gt 0) { [void]$buf.Append($sp.ReadExisting()) }
            Start-Sleep -Milliseconds 50
        } while ((Get-Date) -lt $stop)
        return $buf.ToString()
    }
    $sp.Write("`r")
    [void]$transcript.Append((ReadFor 2))
    $token = 'CUPID_ID_' + [guid]::NewGuid().ToString('N')
    $sp.Write("printf '\n$token\n'; tr '\000' '\n' < /proc/device-tree/compatible; uname -r; printf '\n${token}_END\n'`r")
    $identity = ReadFor 3
    [void]$transcript.Append($identity)
    if ($identity -notmatch '(?m)^xiaomi,cupid\r?$' -or $identity -notmatch '(?m)^7\.[^\r\n]*cupid[^\r\n]*\r?$') { throw 'Linux board/kernel identity verification failed.' }
    $sp.Write("stty -echo; unset PROMPT_COMMAND; export TERM=dumb PS1='cupid-debug# '`r")
    [void]$transcript.Append((ReadFor 1))
    $command = (Get-Content $CommandFile -Raw).Replace("`r",'').Trim()
    if ($command -match "[^\u0009\u000A\u000D\u0020-\u007E]") { throw 'Serial commands must be ASCII.' }
    $payload = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($command))
    $tag = 'CUPID_RUN_' + [guid]::NewGuid().ToString('N')
    $wire = "printf '\n${tag}_BEGIN\n'; __cupid_script=`$(printf '%s' '$payload' | { if command -v base64 >/dev/null 2>&1; then base64 -d; else /bin/busybox base64 -d; fi; }); __cupid_rc=`$?; if [ `"`$__cupid_rc`" -eq 0 ]; then /bin/sh -c `"`$__cupid_script`"; __cupid_rc=`$?; fi; printf '\n${tag}_END=%s\n' `"`$__cupid_rc`""
    if ($payload.Length -gt 2200) {
        $upload = '/run/' + $tag + '.b64'
        $sp.Write("umask 077; : > '$upload'`r")
        [void]$transcript.Append((ReadFor 0.2))
        for ($offset = 0; $offset -lt $payload.Length; $offset += 1200) {
            $chunk = $payload.Substring($offset,[Math]::Min(1200,$payload.Length-$offset))
            $ack = $tag + '_PART_' + $offset
            $sp.Write("printf '%s' '$chunk' >> '$upload'; printf '\n$ack\n'`r")
            $reply = ReadFor 0.5
            [void]$transcript.Append($reply)
            if ($reply -notmatch "(?m)^$ack\r?$" ) { throw 'Serial script upload was not acknowledged.' }
        }
        $wire = "printf '\n${tag}_BEGIN\n'; __cupid_script=`$( { if command -v base64 >/dev/null 2>&1; then base64 -d; else /bin/busybox base64 -d; fi; } < '$upload'); __cupid_rc=`$?; if [ `"`$__cupid_rc`" -eq 0 ]; then /bin/sh -c `"`$__cupid_script`"; __cupid_rc=`$?; fi; rm -f '$upload'; printf '\n${tag}_END=%s\n' `"`$__cupid_rc`""
    }
    [void]$transcript.AppendLine("COMMAND_FILE=$CommandFile")
    $sp.Write($wire + "`r")
    $result = [System.Text.StringBuilder]::new()
    $until = (Get-Date).AddSeconds($TimeoutSeconds)
    $complete = $false
    do {
        if ($sp.BytesToRead -gt 0) { [void]$result.Append($sp.ReadExisting()) }
        if ($result.ToString() -match "(?m)^${tag}_END=(\d+)\r?$" ) {
            $rc = [int]$Matches[1]
            $complete = $true
            break
        }
        Start-Sleep -Milliseconds 50
    } while ((Get-Date) -lt $until)
    [void]$transcript.Append($result.ToString())
    if ($CaptureBase64) {
        $body = $result.ToString()
        if ($body -notmatch '(?s)CUPID_BASE64_BEGIN\s+([A-Za-z0-9+/=\r\n]+)\s+CUPID_BASE64_END') { throw 'Complete base64 capture markers were not received.' }
        $bytes = [Convert]::FromBase64String(($Matches[1] -replace '\s',''))
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actualHash = ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','').ToLower() } finally { $sha.Dispose() }
        if ($body -notmatch '(?m)^([a-f0-9]{64})[ \t]+/run/user/1000/cupid-desktop\.png\r?$' -or $Matches[1] -ne $actualHash) { throw 'Screenshot transfer checksum mismatch; not saving corrupt image.' }
        if (Test-Path $CaptureBase64) { throw 'Capture destination already exists; choose a new filename.' }
        [IO.File]::WriteAllBytes($CaptureBase64,$bytes)
        "CAPTURE=$CaptureBase64; SHA256=$actualHash"
    } else {
        $clean = $result.ToString() -replace '\x1b\[[0-9;?]*[a-zA-Z]','' -replace '\x1b\][^\x07\x1b]*(?:\x07|\x1b\\)',''
        $clean -replace [regex]::Escape($t.serial), '[device]'
    }
    if (-not $complete) { throw 'Serial command timed out; partial transcript retained.' }
    if ($rc -ne 0) { throw "Device command failed with exit status $rc" }
} finally {
    if ($transcript) { [IO.File]::WriteAllText($log,$transcript.ToString(),[Text.UTF8Encoding]::new($false)) }
    if ($sp -and $sp.IsOpen) { $sp.Close() }
    if ($sp) { $sp.Dispose() }
    $mutex.ReleaseMutex()
    $mutex.Dispose()
    "LOG=$log"
}
