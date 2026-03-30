@echo off
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "REPO_ROOT=%%~fI"

set "QT_VERSION=6.11.0"
set "EIGEN_VERSION=5.0.1"
set "LINKAGE=dynamic"
set "QT_DIR="
set "EIGEN_DIR="
set "REPOSITORY=mne-tools/mne-cpp"
set "QT_RELEASE_TAG="
set "EIGEN_RELEASE_TAG="
set "FORCE=0"
set "BUNDLED_EIGEN_DIR="
set "SKIP_QT=0"

:parse_args
if "%~1"=="" goto after_parse
if /I "%~1"=="--qt-version" (
    set "QT_VERSION=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--eigen-version" (
    set "EIGEN_VERSION=%~2"
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
if /I "%~1"=="--qt-dir" (
    set "QT_DIR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--eigen-dir" (
    set "EIGEN_DIR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--skip-qt" (
    set "SKIP_QT=1"
    shift
    goto parse_args
)
if /I "%~1"=="--repository" (
    set "REPOSITORY=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--qt-release-tag" (
    set "QT_RELEASE_TAG=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--eigen-release-tag" (
    set "EIGEN_RELEASE_TAG=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--force" (
    set "FORCE=1"
    shift
    goto parse_args
)
if /I "%~1"=="--help" goto usage_ok
if /I "%~1"=="-h" goto usage_ok

echo Unknown option: %~1 1>&2
goto usage_fail

:after_parse
if /I not "%LINKAGE%"=="dynamic" if /I not "%LINKAGE%"=="static" (
    echo --linkage must be either "dynamic" or "static" 1>&2
    exit /b 1
)

if not defined QT_DIR set "QT_DIR=%SCRIPT_DIR%\qt\%LINKAGE%"
if not defined EIGEN_DIR set "EIGEN_DIR=%SCRIPT_DIR%\eigen"
set "BUNDLED_EIGEN_DIR=%SCRIPT_DIR%\eigen-%EIGEN_VERSION%"

if not exist "%SCRIPT_DIR%\qt" mkdir "%SCRIPT_DIR%\qt"

set "QT_CONFIG=%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake"
set "EIGEN_CONFIG=%EIGEN_DIR%\share\eigen3\cmake\Eigen3Config.cmake"

if "%SKIP_QT%"=="1" goto skip_qt
if "%FORCE%"=="1" goto download_qt
if exist "%QT_CONFIG%" (
    echo Qt bundle already present at %QT_DIR%
    goto after_qt
)

:download_qt
call "%REPO_ROOT%\scripts\ci\download_toolchain.bat" --kind qt --version "%QT_VERSION%" --linkage "%LINKAGE%" --output-dir "%QT_DIR%" --repository "%REPOSITORY%" --release-tag "%QT_RELEASE_TAG%"
if errorlevel 1 exit /b 1
goto after_qt

:skip_qt
if defined QT_DIR (
    echo Skipping Qt artifact download. Using caller-provided/local Qt at %QT_DIR%
) else (
    echo Skipping Qt artifact download. Expecting Qt to be supplied by the caller.
)

:after_qt
if "%FORCE%"=="1" goto download_eigen
if exist "%EIGEN_CONFIG%" (
    echo Eigen bundle already present at %EIGEN_DIR%
    goto after_eigen
)

:download_eigen
call "%REPO_ROOT%\scripts\ci\download_toolchain.bat" --kind eigen --version "%EIGEN_VERSION%" --output-dir "%EIGEN_DIR%" --repository "%REPOSITORY%" --release-tag "%EIGEN_RELEASE_TAG%"
if errorlevel 1 (
    if exist "%BUNDLED_EIGEN_DIR%\CMakeLists.txt" (
        echo Eigen artifact is not available in the public prerelease yet. Continuing with bundled fallback at %BUNDLED_EIGEN_DIR%.
    ) else (
        echo Failed to prepare Eigen and no bundled fallback was found at %BUNDLED_EIGEN_DIR%. 1>&2
        exit /b 1
    )
)

:after_eigen
echo.
echo Dependency setup complete.
if "%SKIP_QT%"=="1" (
    echo   Qt:    %QT_DIR% ^(caller-provided/local^)
) else (
    echo   Qt:    %QT_DIR%
)
if exist "%EIGEN_DIR%\share\eigen3\cmake\Eigen3Config.cmake" (
    echo   Eigen: %EIGEN_DIR%
    echo   CMake prefix hint: %QT_DIR%;%EIGEN_DIR%
) else (
    echo   Eigen: bundled fallback at %BUNDLED_EIGEN_DIR%
    echo   CMake prefix hint: %QT_DIR%
)
exit /b 0

:usage_ok
call :usage
exit /b 0

:usage_fail
call :usage
exit /b 1

:usage
echo Usage: src\external\init.bat [options]
echo.
echo Downloads the MNE-CPP-maintained Qt and Eigen dependency bundles into src\external.
echo.
echo Options:
echo   --qt-version ^<version^>        Qt version to download ^(default: 6.11.0^)
echo   --eigen-version ^<version^>     Eigen version to download ^(default: 5.0.1^)
echo   --linkage ^<dynamic^|static^>   Qt linkage to prepare ^(default: dynamic^)
echo   --qt-dir ^<path^>               Target directory for the Qt bundle
echo   --eigen-dir ^<path^>            Target directory for the Eigen bundle
echo   --skip-qt                       Skip Qt download and keep using the provided/local Qt prefix
echo   --repository ^<owner/repo^>     Repository hosting the prerelease assets ^(default: mne-tools/mne-cpp^)
echo   --qt-release-tag ^<tag^>        Override the Qt prerelease tag
echo   --eigen-release-tag ^<tag^>     Override the Eigen prerelease tag
echo   --force                         Re-download even if the expected package already exists
echo   --help                          Show this help text
exit /b 0
