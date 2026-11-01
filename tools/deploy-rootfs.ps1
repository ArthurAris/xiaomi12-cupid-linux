param(
    [Parameter(Mandatory=$true)][string]$Image,
    [switch]$AllowLowBattery
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$t = Get-Content "$root\private\device-target.json" -Raw | ConvertFrom-Json
$mutex = New-Object System.Threading.Mutex($false, "Local\cupid-device-$($t.serial)")
if (-not $mutex.WaitOne(0)) { throw 'Another device operator is active.' }
$stamp = Get-Date -Format yyyyMMdd-HHmmss
$log = "$root\private\logs\deploy-rootfs-$stamp.txt"
$backupDir = "$root\private\backup\pre-deploy-$stamp"

function A([string[]]$Command) {
    $ErrorActionPreference = 'Continue'
    $r = & $t.adb -s $t.serial @Command 2>&1
    $rc = $LASTEXITCODE
    $r | Out-File $log -Append -Encoding UTF8
    if ($rc -ne 0) { throw "ADB failed: $($Command[0]) rc=$rc" }
    return ($r -join "`n").Trim()
}
function Capture([string]$RemoteCommand, [string]$Destination) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $t.adb
    $psi.Arguments = "-s $($t.serial) exec-out $RemoteCommand"
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $p = [System.Diagnostics.Process]::Start($psi)
    $err = $p.StandardError.ReadToEndAsync()
    $f = [System.IO.File]::Open($Destination, [System.IO.FileMode]::CreateNew)
    try { $p.StandardOutput.BaseStream.CopyTo($f) } finally { $f.Dispose() }
    $p.WaitForExit()
    $err.Result | Out-File $log -Append -Encoding UTF8
    if ($p.ExitCode -ne 0) { throw 'Binary ADB capture failed.' }
}

try {
    if (-not (Test-Path $Image)) { throw 'Image not found.' }
    if ((Get-Item $Image).Length -ne 2147483648) { throw 'Root image must be exactly 2 GiB.' }
    $newHash = (Get-FileHash $Image -Algorithm SHA256).Hash.ToLower()
    $shaFile = "$Image.sha256"
    if (Test-Path $shaFile) {
        $expected = ((Get-Content $shaFile -Raw).Trim() -split '\s+')[0].ToLower()
        if ($newHash -ne $expected) { throw 'Image does not match its .sha256 file.' }
    }

    $state = A @('get-state')
    $identity = A @('shell','getprop ro.boot.hardware.sku; id')
    if ($state -ne 'recovery' -or $identity -notmatch '(?m)^cupid\r?$' -or $identity -notmatch 'uid=0') {
        throw 'Recovery identity gate failed (needs TWRP recovery as root).'
    }
    $battery = [int](A @('shell','cat /sys/class/power_supply/battery/capacity'))
    if ($battery -lt 40) {
        if (-not $AllowLowBattery) { throw "Battery below 40 percent: $battery" }
        "USER-AUTHORIZED LOW BATTERY: $battery percent" | Tee-Object -FilePath $log -Append
    }
    $geometry = A @('shell','readlink -f /dev/block/by-name/cust; blockdev --getsize64 /dev/block/by-name/cust; cat /proc/mounts')
    if ($geometry -notmatch '(?m)^/dev/block/sda27\r?$' -or $geometry -notmatch '(?m)^2147483648\r?$') {
        throw 'cust partition identity mismatch.'
    }
    if ($geometry -match '(?m)^/dev/block/sda27[ \t]+/') { throw 'cust must be unmounted.' }

    New-Item -ItemType Directory -Path $backupDir | Out-Null
    $before = ((A @('shell','sha256sum /dev/block/by-name/cust')) -split '\s+')[0].ToLower()
    if ($before -notmatch '^[a-f0-9]{64}$') { throw 'Device checksum unavailable.' }
    'Backing up the current 2 GiB root partition.'
    Capture 'cat /dev/block/by-name/cust' "$backupDir\cust-current.img"
    if ((Get-Item "$backupDir\cust-current.img").Length -ne 2147483648) { throw 'Backup size mismatch.' }
    $hostHash = (Get-FileHash "$backupDir\cust-current.img" -Algorithm SHA256).Hash.ToLower()
    $after = ((A @('shell','sha256sum /dev/block/by-name/cust')) -split '\s+')[0].ToLower()
    if ($hostHash -ne $before -or $after -ne $before) { throw 'Backup checksum mismatch; nothing written.' }
    @{ name='cust-current.img'; bytes=2147483648; sha256=$hostHash } | ConvertTo-Json |
        Set-Content "$backupDir\manifest.json" -Encoding UTF8
    "Backup verified: $hostHash"

    $remote = '/tmp/cupid-root-' + $newHash.Substring(0,12) + '.img'
    $probe = A @('shell',"if [ -e '$remote' ]; then sha256sum '$remote'; else printf 'ABSENT\n'; fi")
    if ($probe -eq 'ABSENT') {
        'Transferring the candidate image into recovery RAM.'
        [void](A @('push',($Image -replace '\\','/'),$remote))
    } elseif (($probe -split '\s+')[0].ToLower() -ne $newHash) {
        throw 'Unexpected existing file in recovery RAM.'
    }
    $remoteHash = ((A @('shell',"sha256sum '$remote'")) -split '\s+')[0].ToLower()
    if ($remoteHash -ne $newHash) { throw 'Transfer checksum failed.' }

    $pre = A @('shell','getprop ro.boot.hardware.sku; readlink -f /dev/block/by-name/cust; cat /proc/mounts')
    if ($pre -notmatch '(?m)^cupid\r?$' -or $pre -notmatch '(?m)^/dev/block/sda27\r?$' -or $pre -match '(?m)^/dev/block/sda27[ \t]+/') {
        throw 'Pre-write revalidation failed.'
    }
    $protected = @('boot_a','boot_b','vendor_boot_a','vendor_boot_b','recovery_a','dtbo_a')
    $protectedBefore = (A @('shell',('sha256sum ' + (($protected | ForEach-Object { "/dev/block/by-name/$_" }) -join ' '))))
    'Writing the root image to cust only.'
    [void](A @('shell',"dd if='$remote' of=/dev/block/by-name/cust bs=4194304 && sync"))
    $written = ((A @('shell','sha256sum /dev/block/by-name/cust')) -split '\s+')[0].ToLower()
    if ($written -ne $newHash) { throw 'Written image checksum mismatch; restore from backup.' }
    $protectedAfter = (A @('shell',('sha256sum ' + (($protected | ForEach-Object { "/dev/block/by-name/$_" }) -join ' '))))
    if ($protectedBefore.Trim() -ne $protectedAfter.Trim()) { throw 'Protected Android partition changed.' }

    @{
        image = $Image; sha256 = $newHash; backup = $backupDir;
        target = 'cust'; bytes = 2147483648;
        protectedAndroidUnchanged = $true; time = (Get-Date -Format o)
    } | ConvertTo-Json | Set-Content "$root\private\last-root-deployment.json" -Encoding UTF8
    'ROOT_IMAGE_WRITE_READBACK_VERIFIED; Android partitions unchanged.'
} finally {
    $mutex.ReleaseMutex()
    $mutex.Dispose()
    "LOG=$log"
}
