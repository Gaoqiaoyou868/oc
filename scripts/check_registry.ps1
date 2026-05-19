# Check registry for TR shortcut keys
$regPaths = @(
    "HKLM:\SOFTWARE\Tangent\TRArch\7.0",
    "HKCU:\SOFTWARE\Tangent\TRArch\7.0",
    "HKLM:\SOFTWARE\Tangent\TArch\30V1",
    "HKLM:\SOFTWARE\Tangent",
    "HKCU:\SOFTWARE\Tangent"
)
foreach ($rp in $regPaths) {
    if (Test-Path $rp) {
        Write-Host "=== $rp ==="
        Get-ItemProperty $rp -ErrorAction SilentlyContinue | Out-Host
        Get-ChildItem $rp -ErrorAction SilentlyContinue | ForEach-Object {
            $name = $_.PSChildName
            Write-Host "  Subkey: $name"
            Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue | Out-Host
        }
    } else {
        Write-Host "NOT EXISTS: $rp"
    }
}
