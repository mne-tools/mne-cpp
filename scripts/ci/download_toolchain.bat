@echo off
setlocal EnableDelayedExpansion

set "KIND="
set "VERSION="
set "LINKAGE="
set "OUTPUT_DIR="
set "RELEASE_TAG="
if defined GITHUB_REPOSITORY (
    set "REPOSITORY=%GITHUB_REPOSITORY%"
) else (
    set "REPOSITORY=mne-tools/mne-cpp"
)

:parse_args
if "%~1"=="" goto after_parse
if /I "%~1"=="--kind" (
    set "KIND=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--version" (
    set "VERSION=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--linkage" (
    set "LINKAGE=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--output-dir" (
    set "OUTPUT_DIR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--release-tag" (
    set "RELEASE_TAG=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--repository" (
    set "REPOSITORY=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--help" goto usage_ok
if /I "%~1"=="-h" goto usage_ok

echo Unknown option: %~1 1>&2
goto usage_fail

:after_parse
if not defined KIND goto usage_fail
if not defined VERSION goto usage_fail
if not defined OUTPUT_DIR goto usage_fail

set "VERSION_TOKEN=%VERSION:.=%"

if /I "%KIND%"=="qt" (
    if not defined RELEASE_TAG (
        if defined QT_TOOLCHAIN_RELEASE_TAG (
            set "RELEASE_TAG=%QT_TOOLCHAIN_RELEASE_TAG%"
        ) else (
            set "RELEASE_TAG=qt_binaries"
        )
    )
    if not defined LINKAGE (
        echo --linkage is required when --kind=qt 1>&2
        exit /b 1
    )
    set "ASSET_NAME=qt6_%VERSION_TOKEN%_%LINKAGE%_binaries_win.zip"
) else if /I "%KIND%"=="ifw" (
    if not defined RELEASE_TAG (
        if defined QT_TOOLCHAIN_RELEASE_TAG (
            set "RELEASE_TAG=%QT_TOOLCHAIN_RELEASE_TAG%"
        ) else (
            set "RELEASE_TAG=qt_binaries"
        )
    )
    set "ASSET_NAME=qt_ifw_%VERSION_TOKEN%_win.zip"
) else if /I "%KIND%"=="eigen" (
    if not defined RELEASE_TAG (
        if defined EIGEN_TOOLCHAIN_RELEASE_TAG (
            set "RELEASE_TAG=%EIGEN_TOOLCHAIN_RELEASE_TAG%"
        ) else (
            set "RELEASE_TAG=eigen_artifacts"
        )
    )
    set "ASSET_NAME=eigen_%VERSION_TOKEN%_any.zip"
) else if /I "%KIND%"=="onnxruntime" (
    if not defined RELEASE_TAG (
        if defined ONNXRUNTIME_TOOLCHAIN_RELEASE_TAG (
            set "RELEASE_TAG=%ONNXRUNTIME_TOOLCHAIN_RELEASE_TAG%"
        ) else (
            set "RELEASE_TAG=onnxruntime_artifacts"
        )
    )
    set "ASSET_NAME=onnxruntime_%VERSION_TOKEN%_windows.zip"
) else (
    echo Unsupported toolchain kind: %KIND% 1>&2
    exit /b 1
)

set "DOWNLOAD_DIR=%TEMP%\mnecpp-toolchain-%RANDOM%%RANDOM%%RANDOM%"
if exist "%DOWNLOAD_DIR%" rmdir /s /q "%DOWNLOAD_DIR%"
mkdir "%DOWNLOAD_DIR%" || exit /b 1

echo Downloading %ASSET_NAME% from release %RELEASE_TAG% (%REPOSITORY%)...
call :download_asset
if errorlevel 1 goto fail

if exist "%OUTPUT_DIR%" rmdir /s /q "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%" || goto fail
tar -xf "%DOWNLOAD_DIR%\%ASSET_NAME%" -C "%OUTPUT_DIR%"
if errorlevel 1 goto fail

if /I "%KIND%"=="qt" (
    set "QT_CONFIG_DIR=%OUTPUT_DIR%\lib\cmake\Qt6"
    if defined CMAKE_PREFIX_PATH (
        set "CMAKE_PREFIX_VALUE=%OUTPUT_DIR%;%CMAKE_PREFIX_PATH%"
    ) else (
        set "CMAKE_PREFIX_VALUE=%OUTPUT_DIR%"
    )
    call :persist_env QT_ROOT_DIR "%OUTPUT_DIR%"
    call :persist_env CMAKE_PREFIX_PATH "!CMAKE_PREFIX_VALUE!"
    if exist "%QT_CONFIG_DIR%\Qt6Config.cmake" (
        call :persist_env Qt6_DIR "%QT_CONFIG_DIR%"
    ) else (
        call :persist_env Qt6_DIR "%OUTPUT_DIR%"
    )
    call :append_path "%OUTPUT_DIR%\bin"
    echo Qt toolchain ready at %OUTPUT_DIR%
) else if /I "%KIND%"=="ifw" (
    call :persist_env QtInstallerFramework_DIR "%OUTPUT_DIR%"
    call :persist_env CPACK_IFW_ROOT "%OUTPUT_DIR%"
    call :append_path "%OUTPUT_DIR%\bin"
    echo Qt Installer Framework ready at %OUTPUT_DIR%
) else if /I "%KIND%"=="onnxruntime" (
    call :persist_env ONNXRUNTIME_ROOT_DIR "%OUTPUT_DIR%"
    echo ONNX Runtime ready at %OUTPUT_DIR%
) else (
    set "EIGEN_CONFIG_DIR=%OUTPUT_DIR%\share\eigen3\cmake"
    if defined CMAKE_PREFIX_PATH (
        set "CMAKE_PREFIX_VALUE=%OUTPUT_DIR%;%CMAKE_PREFIX_PATH%"
    ) else (
        set "CMAKE_PREFIX_VALUE=%OUTPUT_DIR%"
    )
    call :persist_env EIGEN3_ROOT_DIR "%OUTPUT_DIR%"
    call :persist_env CMAKE_PREFIX_PATH "!CMAKE_PREFIX_VALUE!"
    if exist "%EIGEN_CONFIG_DIR%\Eigen3Config.cmake" (
        call :persist_env Eigen3_DIR "%EIGEN_CONFIG_DIR%"
    ) else (
        call :persist_env Eigen3_DIR "%OUTPUT_DIR%"
    )
    echo Eigen package ready at %OUTPUT_DIR%
)

if exist "%DOWNLOAD_DIR%" rmdir /s /q "%DOWNLOAD_DIR%"
exit /b 0

:download_asset
set "DL_ATTEMPT=0"
set "DL_DELAY=10"

:download_retry
set /a DL_ATTEMPT+=1

where gh >nul 2>&1
if not errorlevel 1 (
    gh release download "%RELEASE_TAG%" -R "%REPOSITORY%" -p "%ASSET_NAME%" -D "%DOWNLOAD_DIR%" >nul 2>nul
    if not errorlevel 1 exit /b 0
)

curl --fail --location --silent --show-error ^
    --output "%DOWNLOAD_DIR%\%ASSET_NAME%" ^
    "https://github.com/%REPOSITORY%/releases/download/%RELEASE_TAG%/%ASSET_NAME%"
if not errorlevel 1 exit /b 0

if %DL_ATTEMPT% lss 4 (
    echo Download attempt %DL_ATTEMPT%/4 failed, retrying in %DL_DELAY%s...
    timeout /t %DL_DELAY% /nobreak >nul
    set /a DL_DELAY*=2
    goto download_retry
)
echo ERROR: Failed to download %ASSET_NAME% after 4 attempts.
exit /b 1

:persist_env
if defined GITHUB_ENV (
    >> "%GITHUB_ENV%" echo %~1=%~2
)
set "%~1=%~2"
exit /b 0

:append_path
if defined GITHUB_PATH (
    >> "%GITHUB_PATH%" echo %~1
)
set "PATH=%~1;%PATH%"
exit /b 0

:usage_ok
call :usage
exit /b 0

:usage_fail
call :usage
exit /b 1

:usage
echo Usage: scripts\ci\download_toolchain.bat --kind ^<qt^|ifw^|eigen^> --version ^<version^> --output-dir ^<dir^> [--linkage ^<dynamic^|static^>] [--release-tag ^<tag^>] [--repository ^<owner/repo^>]
exit /b 0

:fail
if exist "%DOWNLOAD_DIR%" rmdir /s /q "%DOWNLOAD_DIR%"
exit /b 1
