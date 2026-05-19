# Check if TGstart created .addin file
$paths = @()
$paths += "$env:APPDATA\Autodesk\Revit\Addins\2018\Tch.xml"
$paths += "$env:ProgramData\Autodesk\Revit\Addins\2018\Tch.xml"

foreach ($p in $paths) {
    if (Test-Path $p) {
        $item = Get-Item $p
        Write-Host "Found: $($item.FullName) ($($item.Length) bytes, $($item.LastWriteTime))"
        Get-Content $p -Encoding UTF8
    } else {
        Write-Host "NOT FOUND: $p"
    }
}

# Also check ALL .addin files in user's Addins folder
$userAddins = "$env:APPDATA\Autodesk\Revit\Addins\2018"
if (Test-Path $userAddins) {
    Get-ChildItem "$userAddins\*.addin", "$userAddins\*.xml" -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host "Addin file: $($_.FullName)"
    }
}
