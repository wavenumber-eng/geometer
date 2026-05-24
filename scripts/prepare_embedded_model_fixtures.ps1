param(
    [string]$SourceDir = "C:\Users\EliHughes\Desktop\loz-old-man\output\megamaid\embedded_models",
    [string]$GeometerExe = ".\dist\native\windows-x64\geometer.exe"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$sourcePath = (Resolve-Path -LiteralPath $SourceDir).Path
$geometerPath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $GeometerExe)).Path
$stepOut = Join-Path $repoRoot "tests\fixtures\step\embedded_models"
$glbOut = Join-Path $repoRoot "tests\fixtures\glb\embedded_models"
$manifestPath = Join-Path $repoRoot "tests\fixtures\embedded_models_manifest.json"

New-Item -ItemType Directory -Force -Path $stepOut | Out-Null
New-Item -ItemType Directory -Force -Path $glbOut | Out-Null

$models = Get-ChildItem -LiteralPath $sourcePath -File |
    Where-Object { $_.Extension -match "^\.(step|stp)$" } |
    Sort-Object Name

$manifest = @()
foreach ($model in $models) {
    $stepDest = Join-Path $stepOut $model.Name
    Copy-Item -LiteralPath $model.FullName -Destination $stepDest -Force

    $glbName = [System.IO.Path]::GetFileNameWithoutExtension($model.Name) + ".glb"
    $glbDest = Join-Path $glbOut $glbName
    Write-Host "Converting $($model.Name) -> $glbName"
    & $geometerPath step-to-glb $stepDest $glbDest
    if ($LASTEXITCODE -ne 0) {
        throw "step-to-glb failed for $($model.Name) with exit code $LASTEXITCODE"
    }

    $stepInfo = Get-Item -LiteralPath $stepDest
    $glbInfo = Get-Item -LiteralPath $glbDest
    $manifest += [ordered]@{
        name = $model.Name
        step = "tests/fixtures/step/embedded_models/$($model.Name)"
        glb = "tests/fixtures/glb/embedded_models/$glbName"
        stepBytes = $stepInfo.Length
        glbBytes = $glbInfo.Length
    }
}

$manifest | ConvertTo-Json -Depth 4 | Set-Content -Path $manifestPath -Encoding utf8
Write-Host "Wrote $manifestPath with $($manifest.Count) models."
