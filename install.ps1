# ===============================================================
# install.ps1 — GC Library Installer (CLion / MinGW Compatible)
# ===============================================================

# -----------------------------
# Make CLion CMake reachable
# -----------------------------
$clionCMakeDir = "C:\Program Files\JetBrains\CLion 2025.2.1\bin\cmake\win\x64\bin"
if (Test-Path $clionCMakeDir) {
    Write-Host "Adding CLion CMake to PATH: $clionCMakeDir"
    $env:PATH = "$clionCMakeDir;$env:PATH"
}

# -----------------------------
# Compiler detection
# -----------------------------
function Detect-Compiler {
    $gccCmd = Get-Command g++ -ErrorAction SilentlyContinue
    if ($gccCmd) {
        $gccDir = Split-Path $gccCmd.Source
        Write-Host "Found g++ at: $gccDir"
        $env:PATH = "$gccDir;$env:PATH"
        return "MinGW Makefiles"
    }

    $msvcCmd = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($msvcCmd) {
        Write-Host "Found MSVC at: $($msvcCmd.Source)"
        return "Visual Studio 17 2022"
    }

    Write-Host "No compiler detected. Please ensure MinGW or MSVC is installed and configured."
    exit 1
}

# -----------------------------
# CMake detection
# -----------------------------
function Detect-CMake {
    $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeCmd) {
        Write-Host "Found CMake at: $($cmakeCmd.Source)"
        return $cmakeCmd.Source
    }

    Write-Host "CMake not found. Please install CMake or use CLion."
    exit 1
}

# -----------------------------
# Paths and variables
# -----------------------------
$generator   = Detect-Compiler
$cmakeExe    = Detect-CMake
$projectDir  = Get-Location
$tempDir     = Join-Path $env:TEMP "GC-LibTemp"
$installDir  = Join-Path $projectDir "GCInstall"
$repoUrl     = "https://github.com/weinstockk/CSC2210GarabageCollector.git"

# -----------------------------
# Cleanup previous folders
# -----------------------------
if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
if (Test-Path $installDir) { Remove-Item -Recurse -Force $installDir }

# -----------------------------
# Clone the repository
# -----------------------------
Write-Host "Cloning GC library from GitHub..."
git clone $repoUrl $tempDir | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Git clone failed. Make sure Git is installed."
    exit 1
}

# -----------------------------
# Build and install
# -----------------------------
Write-Host "Configuring and building GC library..."
Set-Location $tempDir
New-Item -ItemType Directory -Force -Name "build" | Out-Null
Set-Location build

& $cmakeExe -G "$generator" -DCMAKE_INSTALL_PREFIX="$installDir" ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed."
    exit 1
}

& $cmakeExe --build . --target install
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed."
    exit 1
}

# -----------------------------
# Update user's CMakeLists.txt
# -----------------------------
$cmakeFile = Join-Path $projectDir "CMakeLists.txt"
if (Test-Path $cmakeFile) {
    Write-Host "Updating CMakeLists.txt to include GC library..."

    Add-Content $cmakeFile ""
    Add-Content $cmakeFile "# --- Added by GC installer ---"
    Add-Content $cmakeFile "set(GC_DIR `"$installDir`")"
    Add-Content $cmakeFile "include_directories(\${GC_DIR}/include)"
    Add-Content $cmakeFile "add_library(GC STATIC IMPORTED)"
    Add-Content $cmakeFile "set_target_properties(GC PROPERTIES IMPORTED_LOCATION \${GC_DIR}/lib/libGC.a)"

    $content = Get-Content $cmakeFile
    for ($i = 0; $i -lt $content.Count; $i++) {
        if ($content[$i] -match "add_executable\((\w+)") {
            $target = $Matches[1]
            Add-Content $cmakeFile "target_link_libraries($target GC)"
            break
        }
    }
}

# -----------------------------
# Clean up temp folder
# -----------------------------
Set-Location $projectDir
if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }

# -----------------------------
# Success
# -----------------------------
Write-Host ""
Write-Host "----------------------------------------"
Write-Host "GC library installed successfully!"
Write-Host "Installed to: $installDir"
Write-Host "----------------------------------------"
