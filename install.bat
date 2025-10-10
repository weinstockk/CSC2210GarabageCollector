@echo off
setlocal enabledelayedexpansion

REM User project directory (default current)
if "%~1"=="" (
    set "USER_PROJECT=%CD%"
) else (
    set "USER_PROJECT=%~1"
)

REM Temporary folder for download/build
set "TMP_DIR=%TEMP%\GC-LibTemp"
if exist "%TMP_DIR%" rmdir /s /q "%TMP_DIR%"

REM Download the repo
git clone https://github.com/username/GC-Lib.git "%TMP_DIR%"

REM Build the library
cd "%TMP_DIR%"
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="%USER_PROJECT%\GCInstall"
cmake --build . --config Release --target install

REM Update user's CMakeLists.txt
set "CMAKE_FILE=%USER_PROJECT%\CMakeLists.txt"
if exist "%CMAKE_FILE%" (
    echo. >> "%CMAKE_FILE%"
    echo # Added by GC installer >> "%CMAKE_FILE%"
    echo list(APPEND CMAKE_PREFIX_PATH "%USER_PROJECT%\GCInstall") >> "%CMAKE_FILE%"
    echo find_package(GC REQUIRED) >> "%CMAKE_FILE%"

    REM Add GC to first executable target
    for /f "tokens=1,2 delims=()" %%a in ('findstr /n "add_executable(" "%CMAKE_FILE%"') do (
        echo target_link_libraries(%%b PRIVATE GC::GC) >> "%CMAKE_FILE%"
        goto :done_target
    )
    :done_target
)

REM Cleanup
rmdir /s /q "%TMP_DIR%"

echo.
echo GC library installed. You can now inherit from GCObject and use GCRef.
pause
