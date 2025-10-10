# ===============================================================
# install.ps1 — Cross-compatible installer for CLion / Windows
# ===============================================================

Write-Host "Detecting available compiler..."

# Try to detect MinGW (g++)
$gcc = Get-Command g++ -ErrorAction SilentlyContinue
$msvc = Get-Command cl.exe -ErrorAction SilentlyContinue
$ninja = Get-Command ninja -ErrorAction SilentlyContinue

$generator = ""
if ($gcc) {
    Write-Host "✅ Detected MinGW (g++): $($gcc.Source)"
    $generator = "MinGW Makefiles"
} elseif ($msvc) {
    Write-Host "✅ Detected MSVC (cl.exe): $($msvc.Source)"
    $generator = "Visual Studio 17 2022"
} elseif ($ninja) {
    Write-Host "✅ No compiler detected yet, but Ninja is available."
    $generator = "Ninja"
} else {
    Write-Host "❌ No supported compiler found."
    Write-Host "Please ensure MinGW or Visual Studio Build Tools are installed and visible in PATH."
    exit 1
}

# --- Variables ---
$temp = "$env:TEMP\GC-LibTemp"
$repo = "https://github.com/weinstockk/CSC2210GarabageCollector.git"
$dest = "C:\Program Files\CSC2210GarbageCollector"

# --- Cleanup previous install ---
if (Test-Path $temp) { Remove-Item -Recurse -Force $temp }
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }

# --- Clone repo ---
Write-Host "Cloning GC library from GitHub..."
git clone $repo $temp

# --- Configure and build ---
Write-Host "Configuring with CMake generator: $generator ..."
cd $temp
mkdir build -Force | Out-Null
cd build

# Build command based on generator
if ($generator -eq "Visual Studio 17 2022") {
    cmake -G "$generator" -DCMAKE_INSTALL_PREFIX="$dest" ..
    cmake --build . --config Release --target install
} else {
    cmake -G "$generator" -DCMAKE_INSTALL_PREFIX="$dest" ..
    cmake --build . --target install
}

# --- Clean up ---
cd ..
Remove-Item -Recurse -Force $temp

Write-Host ""
Write-Host "----------------------------------------"
Write-Host "✅ GC library installed successfully!"
Write-Host "📁 Installed to: $dest"
Write-Host "----------------------------------------"
