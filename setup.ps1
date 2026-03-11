$distDir = Join-Path $PSScriptRoot "dist"

if (-not (Test-Path $distDir))
{
    Write-Host "dist/ directory not found. Run a build first." -ForegroundColor Yellow
    return
}

if ($env:PATH -split ";" | Where-Object { $_ -eq $distDir })
{
    Write-Host "dist/ already in PATH." -ForegroundColor Cyan
}
else
{
    $env:PATH = "$distDir;$env:PATH"
    Write-Host "Added $distDir to PATH for this session." -ForegroundColor Green
}
