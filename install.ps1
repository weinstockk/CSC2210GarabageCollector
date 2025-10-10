# Variables
$repoUrl = "https://github.com/weinstockk/CSC2210GarabageCollector.git"
$projectDir = $PWD
$installDir = Join-Path $projectDir "GCInstall"
$tempDir = Join-Path $env:TEMP "GC-LibTemp"

# Clean previous temp folder
if (Test-Path $tempDir) { Remove-Item $tempDir -Recurse -Force }

# Clone repo to temp folder
git clone $repoUrl $tempDir

# Build library
$buildDir = Join-Path $tempDir "build"
New-Item -ItemType Directory -Path $buildDir | Out-Null
Set-Location $buildDir
cmake .. -DCMAKE_INSTALL_PREFIX=$installDir
cmake --build . --config Release --target install

# Update user CMakeLists.txt
$cmakeFile = Join-Path $projectDir "CMakeLists.txt"
if (Test-Path $cmakeFile) {
    Add-Content $cmakeFile ""
    Add-Content $cmakeFile "# Added by GC installer"
    Add-Content $cmakeFile "list(APPEND CMAKE_PREFIX_PATH `"$installDir`")"
    Add-Content $cmakeFile "find_package(GC REQUIRED)"

    # Find first add_executable line and add target_link_libraries
    $content = Get-Content $cmakeFile
    for ($i=0; $i -lt $content.Count; $i++) {
        if ($content[$i] -match "add_executable\((\w+)") {
            $target = $Matches[1]
            Add-Content $cmakeFile "target_link_libraries($target PRIVATE GC::GC)"
            break
        }
    }
}

# Cleanup
Remove-Item $tempDir -Recurse -Force

Write-Host "GC library installed successfully!"
Write-Host "You can now inherit from GCObject and use GCRef."
