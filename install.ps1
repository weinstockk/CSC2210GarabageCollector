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

    function AskForPath([string]$description) {
        while ($true) {
            $userPath = Read-Host "Enter the full path to $description (or type 'exit' to cancel)"
            if ($userPath -eq 'exit') {
                Write-Host "Installation aborted by user."
                exit 1
            }
            if (Test-Path $userPath) {
                Write-Host "$description found at: $userPath"
                return $userPath
            } else {
                Write-Host "Path not found. Please try again."
            }
        }
    }

    # Ask user which compiler to use
    Write-Host "`nSelect a compiler:"
    Write-Host "1) MinGW (g++)"
    Write-Host "2) MSVC (cl.exe)"
    $choice = Read-Host "Enter choice (1 or 2)"

    if ($choice -eq "1") {
        $gccCmd = Get-Command g++ -ErrorAction SilentlyContinue
        if ($gccCmd) {
            $gccDir = Split-Path $gccCmd.Source
            Write-Host "Found g++ at: $gccDir"
            $env:PATH = "$gccDir;$env:PATH"
            return "MinGW Makefiles"
        } else {
            Write-Host "g++ not found. Please install MinGW or provide its path."
            $gccDir = AskForPath "MinGW bin directory (where g++.exe is)"
            $env:PATH = "$gccDir;$env:PATH"
            return "MinGW Makefiles"
        }
    }

    elseif ($choice -eq "2") {
        # --- Locate vcvars64.bat ---
        $vsBase = "C:\Program Files\Microsoft Visual Studio\2022"
        $vsEditions = @("Community", "Professional", "Enterprise")
        $vcvarsPath = $null

        foreach ($edition in $vsEditions) {
            $tryPath = Join-Path $vsBase "$edition\VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $tryPath) {
                $vcvarsPath = $tryPath
                break
            }
        }

        if (-not $vcvarsPath) {
            Write-Host "Could not automatically locate vcvars64.bat."
            $vcvarsPath = AskForPath "vcvars64.bat for Visual Studio"
        }

        Write-Host "Found vcvars64.bat at: $vcvarsPath"

        # --- Load VS environment into PowerShell ---
        Write-Host "Loading Visual Studio environment..."
        cmd /c "call `"$vcvarsPath`" && set" | ForEach-Object {
            if ($_ -match '^(.*?)=(.*)$') {
                $name = $matches[1]
                $value = $matches[2]
                Set-Item -Path "Env:$name" -Value $value
            }
        }

        # --- Refresh PATH and search manually for cl.exe ---
        Write-Host "Refreshing PATH environment..."
        $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Process")

        $clPath = ($env:PATH -split ';' | Where-Object { Test-Path (Join-Path $_ 'cl.exe') } | Select-Object -First 1)

        if ($clPath) {
            Write-Host "Found cl.exe at: $clPath"
            return "Visual Studio 17 2022"
        } else {
            Write-Host "vcvars64.bat loaded, but cl.exe still not found in PATH."
            Write-Host "Try launching 'Developer Command Prompt for VS 2022' manually to verify."
            exit 1
        }
    }

    else {
        Write-Host "Invalid choice. Please select 1 or 2."
        exit 1
    }
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
git checkout --quiet tags/v1.1 | Out-Null # Update Tag When releasing
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
