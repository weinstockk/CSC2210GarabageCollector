# -----------------------------
# PowerShell GC Library Installer
# -----------------------------

# User project directory (where CMakeLists.txt is)
$projectDir = $PWD

# Temporary folder for cloning
$tempDir = Join-Path $env:TEMP "GC-LibTemp"

# Install folder inside the project
$installDir = Join-Path $projectDir "GCInstall"

# GitHub repo
$repoUrl = "https://github.com/weinstockk/CSC2210GarabageCollector.git"

# -----------------------------
# Cleanup previous temp folder
# -----------------------------
if (Test-Path $tempDir) { Remove-Item $tempDir -Recurse -Force }

# -----------------------------
# Clone the repository
# -----------------------------
git clone $repoUrl $tempDir

# -----------------------------
# Build and install with CMake
# -----------------------------
$buildDir = Join-Path $tempDir "build"
New-Item -ItemType Directory -Path $buildDir | Out-Null
Set-Location $buildDir

# Detect CMake from CLion (adjust path if needed)
$cmakeExe = "C:\Program Files\JetBrains\CLion 2025.2.1\bin\cmake\win\x64\bin\cmake.exe"
if (-Not (Test-Path $cmakeExe)) {
    Write-Host "CMake not found at default CLion path. Make sure cmake.exe is installed and adjust the path in this script."
    exit 1
}

# Convert Windows backslashes to forward slashes for CMake
$installDirCMake = $installDir -replace '\\','/'

# Configure and build
& $cmakeExe .. -DCMAKE_INSTALL_PREFIX=$installDirCMake
& $cmakeExe --build . --config Release --target install

# -----------------------------
# Update user's CMakeLists.txt
# -----------------------------
$cmakeFile = Join-Path $projectDir "CMakeLists.txt"
if (Test-Path $cmakeFile) {
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
# Cleanup
# -----------------------------
Remove-Item $tempDir -Recurse -Force

Write-Host "GC library installed successfully!"
Write-Host "You can now inherit from GCObject and use GCRef."
