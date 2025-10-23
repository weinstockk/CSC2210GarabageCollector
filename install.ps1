# ===============================================================
# install.ps1 — GC Library Installer (CLion / MinGW / MSVC Compatible)
# ===============================================================
# Usage: Run from your project folder. This script temporarily configures
# the environment to detect MSVC (vcvars64.bat) if needed and builds
# the GC library via CMake. It updates your CMakeLists.txt to link the
# correct library file for the detected compiler.
# ---------------------------------------------------------------

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# -----------------------------
# Interactive path check function
# -----------------------------
function Confirm-Or-AskPath([string]$description, [string]$defaultPath) {
    if ($defaultPath -and (Test-Path $defaultPath)) {
        $response = Read-Host "$description detected at $defaultPath. Use this path? (y/n)"
        if ($response -match '^[Yy]') {
            return $defaultPath
        }
    } else {
        Write-Host "$description not found at default path: $defaultPath"
    }

    while ($true) {
        $userPath = Read-Host "Enter the correct path for $description (or type 'exit' to cancel)"
        if ($userPath -eq 'exit') {
            Write-Host "Installation aborted by user."
            exit 1
        }
        if (Test-Path $userPath) { return $userPath }
        Write-Host "Path does not exist. Please try again."
    }
}

# -----------------------------
# Detect Compiler (interactive & automatic)
# -----------------------------
function DetectCompiler {
    Write-Host "`n=== Detecting compiler ==="

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

    # Detect MinGW g++
    $gccCmd = Get-Command g++ -ErrorAction SilentlyContinue
    $foundGCC = $false
    if ($gccCmd) {
        $gccDir = Split-Path $gccCmd.Source
        Write-Host "Found MinGW (g++) at: $gccDir"
        $foundGCC = $true
    }

    # Detect cl.exe
    $msvcCmd = Get-Command cl.exe -ErrorAction SilentlyContinue
    $foundMSVC = $false
    $vcvarsPath = $null

    if ($msvcCmd) {
        Write-Host "Found MSVC (cl.exe) in PATH at: $($msvcCmd.Source)"
        $foundMSVC = $true
    } else {
        # Try to locate vcvars64.bat from common VS 2022 editions
        $vsBase = "C:\Program Files\Microsoft Visual Studio\2022"
        $vsEditions = @("Community","Professional","Enterprise")
        foreach ($edition in $vsEditions) {
            $tryPath = Join-Path $vsBase "$edition\VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $tryPath) {
                $vcvarsPath = $tryPath
                break
            }
        }
        if ($vcvarsPath) {
            Write-Host "Found vcvars64.bat at: $vcvarsPath"
            # Load MSVC env into current PowerShell session (temporary)
            $cmd = 'call "' + $vcvarsPath + '" ^&^& set'
            cmd /c $cmd | ForEach-Object {
                if ($_ -match '^(.*?)=(.*)$') {
                    $n = $matches[1]
                    $v = $matches[2]
                    Set-Item -Path "Env:$n" -Value $v
                }
            }
            # re-check
            $msvcCmd = Get-Command cl.exe -ErrorAction SilentlyContinue
            if ($msvcCmd) {
                Write-Host "MSVC (cl.exe) detected after loading vcvars."
                $foundMSVC = $true
            }
        }
    }

    # If both detected, ask user to choose
    if ($foundGCC -and $foundMSVC) {
        Write-Host "`nMultiple compilers detected:"
        Write-Host "  1) MinGW (g++)"
        Write-Host "  2) MSVC (cl.exe)"
        while ($true) {
            $choice = Read-Host "Select a compiler (1 or 2) [default 1]"
            if ([string]::IsNullOrWhiteSpace($choice)) { $choice = '1' }
            if ($choice -in @('1','2')) { break }
            Write-Host "Invalid choice. Enter 1 or 2."
        }
        if ($choice -eq '2') {
            Write-Host "User selected MSVC."
            return "Visual Studio 17 2022"
        } else {
            Write-Host "User selected MinGW (g++)."
            $env:PATH = "$gccDir;$env:PATH"
            return "MinGW Makefiles"
        }
    }

    if ($foundGCC) {
        Write-Host "Using MinGW (g++)"
        $env:PATH = "$gccDir;$env:PATH"
        return "MinGW Makefiles"
    }

    if ($foundMSVC) {
        Write-Host "Using MSVC (cl.exe)"
        return "Visual Studio 17 2022"
    }

    # If nothing detected, ask user to provide either g++ or vcvars64.bat
    Write-Host "No compilers automatically detected."
    Write-Host "You can either provide path to a g++ executable or to vcvars64.bat for Visual Studio."
    $manual = Read-Host "Type 'g' to provide g++ path, 'v' to provide vcvars64.bat, or 'exit' to abort"
    if ($manual -eq 'exit') { Write-Host "Aborted."; exit 1 }

    if ($manual -eq 'g') {
        $gpath = AskForPath "g++ executable (full path)"
        $gdir = Split-Path $gpath
        $env:PATH = "$gdir;$env:PATH"
        return "MinGW Makefiles"
    } elseif ($manual -eq 'v') {
        $vcvarsPath = AskForPath "vcvars64.bat for Visual Studio"
        $cmd = 'call "' + $vcvarsPath + '" ^&^& set'
        cmd /c $cmd | ForEach-Object {
            if ($_ -match '^(.*?)=(.*)$') {
                Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
            }
        }
        # verify
        $msvcCmd = Get-Command cl.exe -ErrorAction SilentlyContinue
        if ($msvcCmd) {
            return "Visual Studio 17 2022"
        } else {
            Write-Error "Failed to load MSVC environment from provided vcvars64.bat."
            exit 1
        }
    } else {
        Write-Error "Unknown option. Aborting."
        exit 1
    }
}

# -----------------------------
# Detect CMake
# -----------------------------
function DetectCMake {
    $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeCmd) {
        Write-Host "Found CMake at: $($cmakeCmd.Source)"
        return $cmakeCmd.Source
    }
    Write-Error "CMake not found. Please install CMake or use CLion."
    exit 1
}

# -----------------------------
# Main script flow
# -----------------------------
try {
    $generator   = DetectCompiler
    $cmakeExe    = DetectCMake
    $projectDir  = Get-Location

    Write-Host "`nUsing CMake generator: $generator"
    Write-Host "Using CMake executable: $cmakeExe"
    Write-Host "Project directory: $projectDir"

    # Temp directory and install dir
    $tempDir = Join-Path $env:TEMP "GC-LibTemp"
    $installDir = Join-Path $projectDir "GCInstall"

    # If existing install dir, take ownership & remove
    if (Test-Path $installDir) {
        Write-Host "Taking ownership of existing GCInstall folder..."
        & takeown /F $installDir /R /D Y | Out-Null
        & icacls $installDir /grant "$($env:USERNAME):(F)" /T /C | Out-Null
        Remove-Item $installDir -Recurse -Force
    }

    # Recreate install dir
    if (-Not (Test-Path $installDir)) { New-Item -ItemType Directory -Force -Path $installDir | Out-Null }
    Write-Host "Installing GC library to: $installDir"

    # Cleanup temp
    if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
    if (Test-Path $installDir) { Remove-Item -Recurse -Force $installDir }

    # Clone repo into temp
    $repoUrl = "https://github.com/weinstockk/CSC2210GarabageCollector.git"
    Write-Host "Cloning GC library from GitHub..."
    git clone $repoUrl $tempDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Git clone failed. Make sure Git is installed."
        exit 1
    }

    # Build
    Write-Host "Configuring and building GC library..."
    if (-Not (Test-Path $tempDir)) { New-Item -ItemType Directory -Force -Path $tempDir | Out-Null }
    Set-Location $tempDir

    # Checkout Generational branch/tag if present
    Write-Host "Checking out Generational"
    git checkout --quiet Generational 2>$null
    # ignore failure to checkout (branch may differ); continue

    $buildDir = Join-Path $tempDir "build"
    if (-Not (Test-Path $buildDir)) { New-Item -ItemType Directory -Force -Path $buildDir | Out-Null }
    Set-Location $buildDir

    # Configure CMake
    & $cmakeExe -G "$generator" -DCMAKE_INSTALL_PREFIX="$installDir" ..
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed."
        exit 1
    }

    # Build & install
    & $cmakeExe --build . --target install
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed."
        exit 1
    }

    # -----------------------------
    # Update user's CMake file (only if not already added)
    # -----------------------------
    $defaultCMakeFile = Join-Path $projectDir "CMakeLists.txt"
    if (-Not (Test-Path $defaultCMakeFile)) {
        Write-Host "CMakeLists.txt not found in project root. Will prompt for path."
        while ($true) {
            $candidate = Read-Host "Enter the path to your CMakeLists.txt (or 'exit')"
            if ($candidate -eq 'exit') { Write-Host "Aborted."; exit 1 }
            if (Test-Path $candidate) { $defaultCMakeFile = $candidate; break }
            Write-Host "File does not exist. Try again."
        }
    }
    $cmakeFile = $defaultCMakeFile
    Write-Host "Using CMake file: $cmakeFile"

    $gcLibFilename = if ($generator -like "Visual Studio*") { "libGC.lib" } else { "libGC.a" }
    $gcMarker = "# --- Added by GC installer ---"

    $content = Get-Content $cmakeFile -Raw
    if ($content -notmatch [regex]::Escape($gcMarker)) {
        $gcDirCMake = $installDir -replace '\\','/'
        $gcBlock = @()
        $gcBlock += $gcMarker
        $gcBlock += "set(GC_DIR `"$gcDirCMake`")"
        $gcBlock += 'include_directories(${GC_DIR}/include)'
        $gcBlock += 'add_library(GC STATIC IMPORTED)'
        $gcBlock += "set_target_properties(GC PROPERTIES IMPORTED_LOCATION \${GC_DIR}/lib/$gcLibFilename)"
        $gcBlockStr = $gcBlock -join "`n"

        Add-Content -Path $cmakeFile -Value "`n$gcBlockStr`n"

        # Detect first add_executable target and link GC
        $fileLines = Get-Content $cmakeFile
        $linked = $false
        foreach ($line in $fileLines) {
            if ($line -match 'add_executable\(\s*([A-Za-z0-9_]+)') {
                $target = $matches[1]
                $linkLine = "target_link_libraries($target GC)"
                if ($content -notmatch [regex]::Escape($linkLine)) {
                    Add-Content -Path $cmakeFile -Value $linkLine
                }
                $linked = $true
                break
            }
        }
        if (-not $linked) { Write-Host "Warning: No executable found to link GC library." }

        Write-Host "GC library block added to CMake file."
    } else {
        Write-Host "CMake file already contains GC installer block — skipping edit."
    }

    # Lock files in install dir (read-only)
    function Lock-AllFiles([string]$path) {
        if (Test-Path $path) {
            Get-ChildItem -Path $path -Recurse -File | ForEach-Object {
                Write-Host "Locking file: $($_.FullName)"
                attrib +R +S $_.FullName
                & icacls $_.FullName /inheritance:r /grant:r "Administrators:R" "Users:R" | Out-Null
            }
        } else {
            Write-Host "Path does not exist: $path"
        }
    }

    Lock-AllFiles $installDir

    # cleanup temp
    Set-Location $projectDir
    if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }

    Write-Host ""
    Write-Host "----------------------------------------"
    Write-Host "GC library installed successfully!"
    Write-Host "Installed to: $installDir"
    Write-Host "----------------------------------------"

    # Self-delete installer
    $scriptPath = $MyInvocation.MyCommand.Path
    Write-Host "Cleaning up installer: $scriptPath"
    Start-Sleep -Seconds 1
    Remove-Item -Path $scriptPath -Force

} catch {
    Write-Error "Installer encountered an error: $_"
    exit 1
}
