$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

$submodule = "external/LightWeightGameFramework"

Write-Host "Checking LWGF working tree..."

git submodule update --init $submodule
if ($LASTEXITCODE -ne 0) {
    exit 1
}

$changes = git -C $submodule status --porcelain

if ($LASTEXITCODE -ne 0) {
    exit 1
}

if ($changes) {
    Write-Host "LWGF has uncommitted changes. Update cancelled."
    exit 1
}

Write-Host "Updating LWGF..."

git submodule update --remote $submodule

if ($LASTEXITCODE -ne 0) {
    exit 1
}

git add $submodule

if ($LASTEXITCODE -ne 0) {
    exit 1
}

Write-Host "LWGF update completed."
git status --short
