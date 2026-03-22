# ============================================================
#  make_release.ps1
#  Empaqueta el build de Release en una carpeta lista para
#  comprimir y subir como GitHub Release.
#
#  Uso:
#    .\scripts\make_release.ps1
#    .\scripts\make_release.ps1 -Version "0.2.0"
# ============================================================

param(
    [string]$Version = "dev"
)

$Root       = Split-Path $PSScriptRoot -Parent
$BuildDir   = Join-Path $Root "x64\Release"
$MediaDir   = Join-Path $Root "Minecraft.Client\Common\Media"
$OutputName = "LegacyCraft-$Version"
$OutputDir  = Join-Path $Root "dist\$OutputName"

# ── Verificar que el build exista ──────────────────────────────
if (-not (Test-Path "$BuildDir\Minecraft.Client.exe")) {
    Write-Error "No se encontro Minecraft.Client.exe en $BuildDir"
    Write-Host  "Compila primero en Release|x64 desde Visual Studio."
    exit 1
}

# ── Limpiar y crear carpeta de salida ─────────────────────────
if (Test-Path $OutputDir) { Remove-Item -Recurse -Force $OutputDir }
New-Item -ItemType Directory -Path $OutputDir | Out-Null

# ── Copiar ejecutable ──────────────────────────────────────────
Copy-Item "$BuildDir\Minecraft.Client.exe" -Destination $OutputDir

# ── Copiar assets de media (Graphics, Audio, etc.) ────────────
if (Test-Path $MediaDir) {
    Copy-Item $MediaDir -Destination "$OutputDir\Media" -Recurse
}

# ── Copiar dlls necesarias desde el build ─────────────────────
Get-ChildItem "$BuildDir\*.dll" | Copy-Item -Destination $OutputDir

# ── Crear ZIP ─────────────────────────────────────────────────
$ZipPath = Join-Path $Root "dist\$OutputName.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath }

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($OutputDir, $ZipPath)

Write-Host ""
Write-Host "Release listo:" -ForegroundColor Green
Write-Host "  Carpeta : $OutputDir"
Write-Host "  ZIP     : $ZipPath"
Write-Host ""
Write-Host "Proximos pasos:" -ForegroundColor Cyan
Write-Host "  1. git tag v$Version"
Write-Host "  2. git push origin v$Version"
Write-Host "  3. Sube $OutputName.zip como asset en GitHub Releases"
