# ===============================================================
# install.ps1 — GC Library Installer (CLion / MinGW Compatible)
# ===============================================================

Write-Host "Detecting available compiler..."

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

    Write-Host "No compiler detected. Please ensure MinGW or MSVC is installed."
    exit 1
}

# --- Detect Generator ---
$generator = Detect-Compiler

# --- Setup paths ---
$temp = "$env:TEMP\GC-LibTemp"
$repo = "https://github.com/weinstockk/CSC2210GarabageCollector.git"
$dest = Join-Path $PSScriptRoot "GC-Library"

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
    Write-Host "CMake configuration failed."
    exit 1
}

cmake --build . --target install
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed."
    exit 1
}

# --- Update user CMakeLists.txt ---
$cmakeFile = Join-Path $PSScriptRoot "CMakeLists.txt"
if (Test-Path $cmakeFile) {
    Write-Host "Updating CMakeLists.txt to include GC library..."

    Add-Content $cmakeFile ""
    Add-Content $cmakeFile "# --- Added by GC installer ---"
    Add-Content $cmakeFile "set(GC_DIR `"$dest`")"
    Add-Content $cmakeFile "include_directories(\${GC_DIR}/include)"
    Add-Content $cmakeFile "add_library(GC STATIC IMPORTED)"
    Add-Content $cmakeFile "set_target_properties(GC PROPERTIES IMPORTED_LOCATION \${GC_DIR}/lib/libGC.a)"

    $content = Get-Content $cmakeFile
    for ($i=0; $i -lt $content.Count; $i++) {
        if ($content[$i] -match "add_executable\((\w+)") {
            $target = $Matches[1]
            Add-Content $cmakeFile "target_link_libraries($target GC)"
            break
        }
    }
}

# --- Clean up temp ---
Set-Location $PSScriptRoot
Remove-Item -Recurse -Force $temp

Write-Host ""
Write-Host "----------------------------------------"
Write-Host "GC library installed successfully!"
Write-Host "Installed to: $dest"
Write-Host "CMakeLists.txt updated to include GC"
Write-Host "----------------------------------------"
