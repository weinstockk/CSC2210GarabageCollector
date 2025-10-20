# ===============================================================
# install.ps1 — GC Library Installer (CLion / MinGW Compatible)
# ===============================================================

# -----------------------------
# Interactive path check function
# -----------------------------
function Confirm-Or-AskPath([string]$description, [string]$defaultPath) {
    if (Test-Path $defaultPath) {
        $response = Read-Host "$description detected at $defaultPath. Use this path? (y/n)"
        if ($response -match '^[Yy]') {
            return $defaultPath
        }
    } else {
        Write-Host "$description not found at default path: $defaultPath"
    }

    while ($true) {
        $userPath = Read-Host "Enter the correct path for $description"
        if (Test-Path $userPath) { return $userPath }
        Write-Host "Path does not exist. Please try again."
    }
}

# -----------------------------
# CLion CMake path (interactive)
# -----------------------------
$defaultClionCMakeDir = "C:\Program Files\JetBrains\CLion 2025.2.1\bin\cmake\win\x64\bin"
$clionCMakeDir = Confirm-Or-AskPath "CLion CMake" $defaultClionCMakeDir
Write-Host "Using CLion CMake: $clionCMakeDir"
$env:PATH = "$clionCMakeDir;$env:PATH"

# -----------------------------
# Compiler detection
# -----------------------------
function DetectCompiler {
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

    Write-Host "No compiler detected. Please install MinGW or MSVC."
    exit 1
}

# -----------------------------
# CMake detection
# -----------------------------
function DetectCMake {
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
$generator   = DetectCompiler
$cmakeExe    = DetectCMake
$projectDir  = Get-Location

# Temp directory (automatic, no user prompt)
$tempDir = Join-Path $env:TEMP "GC-LibTemp"

# Install directory (automatic, no user prompt)
$installDir = Join-Path $projectDir "GCInstall"

if (Test-Path $installDir) {
    Write-Host "Taking ownership of existing GCInstall folder..."

    # Take ownership recursively
    takeown /F $installDir /R /D Y | Out-Null

    # Grant full control to the current logged-in user
    icacls $installDir /grant "$($env:USERNAME):(F)" /T /C | Out-Null

    # Remove folder recursively
    Remove-Item $installDir -Recurse -Force
}

# Recreate install directory if it doesn't exist
if (-Not (Test-Path $installDir)) { New-Item -ItemType Directory -Force -Path $installDir | Out-Null }
Write-Host "Installing GC library to: $installDir"

# Repository URL
$repoUrl = "https://github.com/weinstockk/CSC2210GarabageCollector.git"

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

# Create temp/build folder
if (-Not (Test-Path $tempDir)) { New-Item -ItemType Directory -Force -Path $tempDir | Out-Null }
Set-Location $tempDir

# Checkout the specific tag
Write-Host "Checking out Generational"
git checkout --quiet Generational | Out-Null # Update Tag When releasing
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to checkout Branch. Make sure the tag exists."
    exit 1
}

$buildDir = Join-Path $tempDir "build"
if (-Not (Test-Path $buildDir)) { New-Item -ItemType Directory -Force -Path $buildDir | Out-Null }
Set-Location $buildDir

# Configure
& $cmakeExe -G "$generator" -DCMAKE_INSTALL_PREFIX="$installDir" ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed."
    exit 1
}

# Build
& $cmakeExe --build . --target install
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed."
    exit 1
}

# -----------------------------
# Locate CMake file (interactive if not default)
# -----------------------------
$defaultCMakeFile = Join-Path $projectDir "CMakeLists.txt"

if (Test-Path $defaultCMakeFile) {
    $cmakeFile = $defaultCMakeFile
    Write-Host "Using default CMake file: $cmakeFile"
} else {
    while ($true) {
        $userFile = Read-Host "CMakeLists.txt not found. Enter the path to your CMake file"
        if (Test-Path $userFile) {
            $cmakeFile = $userFile
            break
        }
        Write-Host "File does not exist. Try again."
    }
}

# -----------------------------
# Update user's CMake file (only if not already added)
# -----------------------------
Write-Host "Updating CMake file to include GC library..."

# Convert Windows backslashes to forward slashes for CMake
$gcDirCMake = $installDir -replace '\\','/'

# Marker to detect if block already exists
$gcMarker = "# --- Added by GC installer ---"

# Read current content
$content = Get-Content $cmakeFile
$cmakeText = $content -join "`n"   # join lines to scan whole file

if ($cmakeText -notmatch [regex]::Escape($gcMarker)) {
    # Build the GC block safely
    $gcBlockLines = @(
        '# --- Added by GC installer ---',
        "set(GC_DIR `"$gcDirCMake`")",
        'include_directories(${GC_DIR}/include)',
        'add_library(GC STATIC IMPORTED)',
        'set_target_properties(GC PROPERTIES IMPORTED_LOCATION ${GC_DIR}/lib/libGC.a)'
    )

    # Append the block to CMakeLists.txt
    Add-Content -Path $cmakeFile -Value $gcBlockLines

    # Detect the first executable and link GC
    $linked = $false
    foreach ($line in $content) {
        if ($line -match 'add_executable\((\w+)') {
            $target = $Matches[1]
            $linkLine = "target_link_libraries($target GC)"


            # Check if target_link_libraries line exists
            if ($cmakeText -notmatch [regex]::Escape($linkLine)) {
                Add-Content -Path $cmakeFile -Value $linkLine
            }

            $linked = $true
            break
        }
    }

    if (-not $linked) {
        Write-Host "Warning: No executable found to link GC library."
    }

    Write-Host "GC library block added to CMake file."
}

# -----------------------------
# Lock all installed files (read-only & protected)
# -----------------------------
function Lock-AllFiles([string]$path) {
    if (Test-Path $path) {
        Get-ChildItem -Path $path -Recurse -File | ForEach-Object {
            Write-Host "Locking file: $($_.FullName)"

            # Set read-only + system attributes
            attrib +R +S $_.FullName

            # Revoke write permissions for all users except administrators
            icacls $_.FullName /inheritance:r /grant:r "Administrators:R" "Users:R" | Out-Null
        }
    } else {
        Write-Host "Path does not exist: $path"
    }
}

# Lock everything under the install directory
Lock-AllFiles $installDir

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

# -----------------------------
# Self-delete the installer
# -----------------------------
$scriptPath = $MyInvocation.MyCommand.Path
Write-Host "Cleaning up installer: $scriptPath"
Start-Sleep -Seconds 1
Remove-Item -Path $scriptPath -Force
