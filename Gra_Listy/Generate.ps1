# Generuje pliki solucji Visual Studio przy pomocy premake5.
# Uzycie: .\Generate.ps1 [akcja]   (domyslnie vs2022, np. vs2026, gmake)
param(
    [string]$Action = "vs2022"
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

# Najpierw lokalny premake5.exe w folderze projektu, potem ten z PATH
$Premake = Join-Path $PSScriptRoot "premake5.exe"
if (-not (Test-Path $Premake)) {
    $Command = Get-Command "premake5" -ErrorAction SilentlyContinue
    if (-not $Command) {
        Write-Error "Nie znaleziono premake5. Umiesc premake5.exe w folderze projektu lub dodaj go do PATH."
    }
    $Premake = $Command.Source
}

& $Premake $Action
exit $LASTEXITCODE
