# ===============================================================
# install.ps1 — GC Library Installer (CLion / MinGW Compatible)
# ===============================================================

function Confirm-Or-AskPath([string]$description, [string]$defaultPath) {
    if (Test-Path $defaultPath) {
        $response = Read-Host "$description detected at $defaultPath. Use this path? (y/n)"
        if ($response -match '^[Yy]') { return $defaultPath }
    } else {
        Write-Host "$description not found at default path: $defaultPath"
    }

    while ($true) {
        $userPath = Read-Host "Enter the correct path for $description"
        if (Test-Path $userPath) { return $userPath }
        Write-Host "Path does not exist. Please try again."
    }
}

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

    Write-Host "No compiler detected. Please install MinGW or MSVC."
    exit 1
}

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
# Setup environment
# -----------------------------
$defaultClionCMakeDir = "C:\Program Files\JetBrains\CLion 2025.2.1\bin\cmake\win\x64\bin"
$clionCMakeDir = Confirm-Or-AskPath "CLion CMake" $defaultClionCMakeDir
$env:PATH = "$clionCMakeDir;$env:PATH"

$generator   = Detect-Compiler
$cmakeExe    = Detect-CMake
$projectDir  = Get-Location
$tempDir     = Join-Path $env:TEMP "GC-LibTemp"
$installDir  = Join-Path $projectDir "GCInstall"

# Cleanup previous
if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
if (Test-Path $installDir) { Remove-Item -Recurse -Force $installDir }

# -----------------------------
# Clone and build
# -----------------------------
$repoUrl = "https://github.com/weinstockk/CSC2210GarabageCollector.git"

Write-Host "Cloning GC library..."
git clone $repoUrl $tempDir | Out-Null

if ($LASTEXITCODE -ne 0) {
    Write-Host "Git clone failed. Make sure Git is installed."
    exit 1
}

# Build
Write-Host "Building GC library..."
Set-Location (Join-Path $tempDir "build")
& $cmakeExe -G "$generator" -DCMAKE_INSTALL_PREFIX="$installDir" ..
& $cmakeExe --build . --target install

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed."
    exit 1
}

# -----------------------------
# Update user's CMakeLists.txt
# -----------------------------
$cmakeFile = Join-Path $projectDir "CMakeLists.txt"
$gcDirCMake = $installDir -replace '\\','/'
$gcMarker = "# --- Added by GC installer ---"

if (Test-Path $cmakeFile) {
    $content = Get-Content $cmakeFile
    $cmakeText = $content -join "`n"

    if ($cmakeText -notmatch [regex]::Escape($gcMarker)) {
        $gcBlockLines = @(
            '# --- Added by GC installer ---',
            "set(GC_DIR `"$gcDirCMake`")",
            'include_directories(${GC_DIR}/include)',
            'add_library(GC STATIC IMPORTED)',
            'set_target_properties(GC PROPERTIES IMPORTED_LOCATION ${GC_DIR}/lib/libGC.a)'
        )
        Add-Content -Path $cmakeFile -Value $gcBlockLines

        foreach ($line in $content) {
            if ($line -match 'add_executable\((\w+)') {
                $target = $Matches[1]
                Add-Content -Path $cmakeFile -Value "target_link_libraries($target GC)"
                break
            }
        }
    }
}

# -----------------------------
# Lock GCRef.h (read-only & protected)
# -----------------------------
$headerPath = Join-Path $installDir "include\GCRef.h"

if (Test-Path $headerPath) {
    Write-Host "Locking installed header: $headerPath"
    attrib +R +S $headerPath

    # Revoke write permissions for all users except administrators
    icacls $headerPath /inheritance:r /grant:r "Administrators:R" "Users:R" | Out-Null
}

# -----------------------------
# Cleanup temp + self-delete
# -----------------------------
Set-Location $projectDir
if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }

Write-Host ""
Write-Host "----------------------------------------"
Write-Host "GC library installed and secured!"
Write-Host "Installed to: $installDir"
Write-Host "Header locked: GCRef.h"
Write-Host "----------------------------------------"

# Self-delete
$scriptPath = $MyInvocation.MyCommand.Path
Start-Sleep -Seconds 1
Remove-Item -Path $scriptPath -Force
