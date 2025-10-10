# -----------------------------
# GC Library Installer (PowerShell)
# -----------------------------

# Save the starting directory
$startDir = Get-Location

# -----------------------------
# Configuration
# -----------------------------
$projectDir = $startDir
$tempDir = Join-Path $env:TEMP "GC-LibTemp"
$installDir = Join-Path $projectDir "GCInstall"
$repoUrl = "https://github.com/weinstockk/CSC2210GarabageCollector.git"

# CLion's bundled CMake path (adjust if needed)
$cmakeExe = "C:\Program Files\JetBrains\CLion 2025.2.1\bin\cmake\win\x64\bin\cmake.exe"
if (-not (Test-Path $cmakeExe)) {
    Write-Host "❌ Could not find CMake at $cmakeExe."
    Write-Host "Please adjust the path in install.ps1 to match your CLion installation."
    exit 1
}

# -----------------------------
# Check for Git
# -----------------------------
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Git is not installed or not in PATH. Please install Git and rerun."
    exit 1
}

# -----------------------------
# Detect available compiler
# -----------------------------
Write-Host "Detecting available compiler..."
$generator = ""
$tryGenerators = @("MinGW Makefiles", "NMake Makefiles", "Ninja")

foreach ($g in $tryGenerators) {
    $output = & $cmakeExe -G "$g" .. 2>&1 | Out-String
    if ($output -notmatch "CMake Error") {
        $generator = $g
        break
    }
}

if ($generator -eq "") {
    Write-Host "⚠ No working compiler toolchain detected."
    Write-Host "Please open CLion, ensure a toolchain is configured (MinGW or MSVC), and rerun this script."
    exit 1
}

Write-Host "✅ Using generator: $generator"

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
    Write-Host "❌ Git clone failed. Make sure Git is installed and accessible."
    exit 1
}

# -----------------------------
# Build & Install
# -----------------------------
$buildDir = Join-Path $tempDir "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}
Set-Location $buildDir

Write-Host "Configuring with CMake..."
$installDirCMake = $installDir -replace '\\','/'
& $cmakeExe .. -G "$generator" -DCMAKE_INSTALL_PREFIX=$installDirCMake
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake configure failed. Make sure your CLion toolchain is working."
    exit 1
}

Write-Host "Building and installing GC library..."
& $cmakeExe --build . --config Release --target install
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake build/install failed."
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
Write-Host "✅ GC library installed successfully!"
Write-Host "You can now include and use GCObject and GCRef in your code."
Write-Host "----------------------------------------"
