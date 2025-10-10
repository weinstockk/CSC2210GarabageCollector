# ===============================================================
# install.ps1 — GC Library Installer (CLion / MinGW Compatible)
# ===============================================================

Write-Host "Detecting available compiler..."

function Detect-Compiler {
    $gccCmd = Get-Command g++ -ErrorAction SilentlyContinue
    if ($gccCmd) {
        $gccDir = Split-Path $gccCmd.Source
        Write-Host "[OK] Found g++ at: $gccDir"
        $env:PATH = "$gccDir;$env:PATH"
        return "MinGW Makefiles"
    }

    $msvcCmd = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($msvcCmd) {
        Write-Host "[OK] Found MSVC at: $($msvcCmd.Source)"
        return "Visual Studio 17 2022"
    }

    $ninjaCmd = Get-Command ninja -ErrorAction SilentlyContinue
    if ($ninjaCmd) {
        Write-Host "[OK] Found Ninja at: $($ninjaCmd.Source)"
        return "Ninja"
    }

    Write-Host "[ERROR] No compiler detected. Please ensure MinGW or MSVC is installed and configured in CLion."
    exit 1
}

# --- Detect Generator ---
$generator = Detect-Compiler

# --- Setup paths ---
$temp = "$env:TEMP\GC-LibTemp"
$repo = "https://github.com/weinstockk/CSC2210GarabageCollector.git"
$dest = "C:\Program Files\CSC2210GarbageCollector"

# --- Cleanup old folders ---
if (Test-Path $temp) { Remove-Item -Recurse -Force $temp }
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }

# --- Clone Repo ---
Write-Host "Cloning GC library from GitHub..."
git clone $repo $temp | Out-Null

# --- Build and Install ---
Write-Host "Configuring with CMake generator: $generator ..."
Set-Location $temp
New-Item -ItemType Directory -Force -Name "build" | Out-Null
Set-Location build

cmake -G "$generator" -DCMAKE_INSTALL_PREFIX="$dest" ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake configuration failed."
    exit 1
}

cmake --build . --target install
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed."
    exit 1
}

# --- Clean up temp ---
Set-Location ..
Remove-Item -Recurse -Force $temp

Write-Host ""
Write-Host "----------------------------------------"
Write-Host "[SUCCESS] GC library installed successfully!"
Write-Host "Installed to: $dest"
Write-Host "----------------------------------------"
