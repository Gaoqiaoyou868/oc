Add-Type -AssemblyName System.Runtime.WindowsRuntime
Add-Type -AssemblyName Windows.Foundation
Add-Type -AssemblyName Windows.Foundation.UniversalApiContract

$lang = [Windows.Globalization.Language]::new("zh-CN")
$engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($lang)
if ($null -eq $engine) {
    $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
    Write-Output "Using default language"
}

Write-Output "OCR Engine created: $($null -ne $engine)"
Write-Output "Language: $($engine.RecognizerLanguage)"
Write-Output "OCR Available: $([Windows.Media.Ocr.OcrEngine]::IsSupported)"
