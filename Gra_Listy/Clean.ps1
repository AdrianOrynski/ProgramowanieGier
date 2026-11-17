# Usuwa wyniki kompilacji oraz pliki wygenerowane przez premake5 / Visual Studio.
# Uzycie: .\Clean.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

# Foldery z wynikami kompilacji i metadanymi Visual Studio
$Folders = @("build", "build-int", ".vs")
foreach ($Folder in $Folders) {
    if (Test-Path $Folder) {
        Remove-Item $Folder -Recurse -Force
        Write-Host "Usunieto $Folder/"
    }
}

# Pliki solucji, projektow i makefile (bez wchodzenia do vendor/)
$Patterns = @("*.sln", "*.slnx", "*.vcxproj", "*.vcxproj.filters", "*.vcxproj.user", "Makefile", "*.make")
$Dirs = @(".", "engine", "game", "tests")
foreach ($Dir in $Dirs) {
    foreach ($Pattern in $Patterns) {
        Get-ChildItem -Path (Join-Path $Dir $Pattern) -File -ErrorAction SilentlyContinue | ForEach-Object {
            Remove-Item $_.FullName -Force
            Write-Host "Usunieto $($_.Name)"
        }
    }
}
