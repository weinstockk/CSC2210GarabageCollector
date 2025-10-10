# -----------------------------
# GC Library Installer (PowerShell)
# -----------------------------

# Save starting directory
$startDir = Get-Location

# -----------------------------
# Configuration
# -----------------------------
$projectDir = $startDir                        # Project folder (where CMakeLists.txt lives)
$tempDir = Join-Path $env:TEMP "GC-LibTemp"    # Temporary clone folder
$installDir = Join-Path $projectDir "GCInstall" # Install folder
$repoUrl = "https://github.com/weinstockk/CSC2210GarabageCollector.git"

# CLion bundled CMake path (adjust if your CLion version/path differs)
$cmakeExe = "C:\Program Files\JetBrains\CLion 2025.2.1\bin\cmake\win\x64\bin\cmake.exe"

if (-Not (Test-Path $cmakeExe)) {
    Write-Host "CMake not found at $cmakeExe. Please install CMake or adjust the path in this script."
    exit 1
}

# -----------------------------
# Check for Git
# -----------------------------
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "Git is not installed or not in PATH. Please install Git."
    exit 1
}

# -----------------------------
# Detect Compiler
# -----------------------------
# Try Visual Studio (MSVC)
$vsGenerator = "Visual Studio 17 2022"   # adjust for your VS version
$msvcInstalled = & $cmakeExe -G "$vsGenerator" .. 2>&1 | Out-String
if ($msvcInstalled -match "CMake Error") {
    # If MSVC not found, try Ninja + MinGW
    $ninjaCheck = Get-Command ninja -ErrorAction SilentlyContinue
    if (-not $ninjaCheck) {
        Write-Host "No supported compiler found. Installing Ninja..."
        winget install --id NinjaBuild.Ninja -e
    }
    $generator = "Ninja"
} else {
    $generator = $vsGenerator
}

# -----------------------------
# Cleanup previous temp folder
# -----------------------------
if (Test-Path $tempDir) {
    Write-Host "Removing previous temp folder..."
    Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}

# -----------------------------
# Clone GC library
# -----------------------------
Write-Host "Cloning GC library from GitHub..."
git clone $repoUrl $tempDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "Git clone failed. Make sure Git is installed and accessible."
    exit 1
}

# -----------------------------
# Build & Install
# -----------------------------
$buildDir = Join-Path $tempDir "build"
if (-Not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}
Set-Location $buildDir

# Configure CMake
Write-Host "Configuring with CMake generator: $generator ..."
$installDirCMake = $installDir -replace '\\','/'
& $cmakeExe .. -G "$generator" -DCMAKE_INSTALL_PREFIX=$installDirCMake
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configure failed."
    exit 1
}

# Build & install
Write-Host "Building and installing GC library..."
& $cmakeExe --build . --config Release --target install
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake build/install failed."
    exit 1
}

# -----------------------------
# Update CMakeLists.txt
# -----------------------------
$cmakeFile = Join-Path $projectDir "CMakeLists.txt"
if (Test-Path $cmakeFile) {
    Write-Host "Updating CMakeLists.txt..."
    Add-Content $cmakeFile ""
    Add-Content $cmakeFile "# --- Added by GC installer ---"
    Add-Content $cmakeFile "list(APPEND CMAKE_PREFIX_PATH `"$installDirCMake`")"
    Add-Content $cmakeFile "find_package(GC REQUIRED)"

    # Find first add_executable line and link GC::GC
    $content = Get-Content $cmakeFile
    for ($i=0; $i -lt $content.Count; $i++) {
        if ($content[$i] -match "add_executable\((\w+)") {
            $target = $Matches[1]
            Add-Content $cmakeFile "target_link_libraries($target PRIVATE GC::GC)"
            break
        }
    }
}

# -----------------------------
# Cleanup temp folder
# -----------------------------
Set-Location $projectDir
if (Test-Path $tempDir) {
    Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Restore starting directory
Set-Location $startDir

Write-Host "----------------------------------------"
Write-Host "GC library installed successfully!"
Write-Host "You can now inherit from GCObject and use GCRef."
Write-Host "----------------------------------------"
