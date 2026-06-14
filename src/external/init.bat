@echo off
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "REPO_ROOT=%%~fI"

rem Central source of truth for external dependency versions (Eigen, ONNX,
rem Skigen). The env file owns the version data; this script owns the policy
rem (e.g. ONNX is opt-in locally). CLI flags below still override the versions.
set "EXTERNAL_DEPS_ENV=%SCRIPT_DIR%\external_deps.env"
set "EIGEN_VERSION="
set "ONNXRUNTIME_VERSION="
if exist "%EXTERNAL_DEPS_ENV%" (
    for /f "usebackq tokens=1,2 delims==" %%A in ("%EXTERNAL_DEPS_ENV%") do (
        set "deps_key=%%A"
        set "deps_key=!deps_key: =!"
        if not "!deps_key!"=="" if not "!deps_key:~0,1!"=="#" set "%%A=%%B"
    )
)

rem Capture the central ONNX version as the default used *when* ONNX is opted
rem into; the empty ONNXRUNTIME_VERSION below remains the "ONNX disabled locally"
rem flag so loading the env file never auto-enables it.
if defined ONNXRUNTIME_VERSION (
    set "ONNXRUNTIME_DEFAULT_VERSION=!ONNXRUNTIME_VERSION!"
) else (
    set "ONNXRUNTIME_DEFAULT_VERSION=1.21.0"
)

set "QT_VERSION=6.11.1"
if not defined EIGEN_VERSION set "EIGEN_VERSION=5.0.1"
set "LINKAGE=dynamic"
set "QT_DIR="
set "EIGEN_DIR="
set "REPOSITORY=mne-tools/mne-cpp"
set "QT_RELEASE_TAG="
set "EIGEN_RELEASE_TAG="
set "FORCE=0"
set "BUNDLED_EIGEN_DIR="
set "SKIP_QT=0"
set "ONNXRUNTIME_VERSION="
set "ONNXRUNTIME_DIR="
set "ONNXRUNTIME_RELEASE_TAG="

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
if /I "%~1"=="--onnxruntime" (
    rem Opt into ONNX using the central default version from external_deps.env.
    set "ONNXRUNTIME_VERSION=!ONNXRUNTIME_DEFAULT_VERSION!"
    shift
    goto parse_args
)
if /I "%~1"=="--onnxruntime-version" (
    set "ONNXRUNTIME_VERSION=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--onnxruntime-dir" (
    set "ONNXRUNTIME_DIR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--onnxruntime-release-tag" (
    set "ONNXRUNTIME_RELEASE_TAG=%~2"
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
if defined ONNXRUNTIME_VERSION if not defined ONNXRUNTIME_DIR set "ONNXRUNTIME_DIR=%SCRIPT_DIR%\onnxruntime"
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
if not defined ONNXRUNTIME_VERSION goto after_onnxruntime
if "%FORCE%"=="1" goto download_onnxruntime
if exist "%ONNXRUNTIME_DIR%\include\onnxruntime_cxx_api.h" (
    echo ONNX Runtime already present at %ONNXRUNTIME_DIR%
    goto after_onnxruntime
)

:download_onnxruntime
call "%REPO_ROOT%\scripts\ci\download_toolchain.bat" --kind onnxruntime --version "%ONNXRUNTIME_VERSION%" --output-dir "%ONNXRUNTIME_DIR%" --repository "%REPOSITORY%" --release-tag "%ONNXRUNTIME_RELEASE_TAG%"
if errorlevel 1 (
    echo Failed to download ONNX Runtime %ONNXRUNTIME_VERSION%. The ml library will build without ONNX support.
)

:after_onnxruntime
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
if defined ONNXRUNTIME_VERSION (
    echo   ONNX Runtime: %ONNXRUNTIME_DIR%
)

rem ── Skigen — scikit-learn algorithms on Eigen (header-only) ──
rem Ref is resolved from the central config scripts\ci\skigen_ref.env.
rem Local setup defaults to the pinned release tag; override for co-development:
rem   set SKIGEN_REF=staging  (track the dev branch) before running init.bat
rem   set SKIGEN_REF=v1.0.0   (any explicit tag/branch)
set "SKIGEN_REF_CONFIG=%SCRIPT_DIR%\..\..\scripts\ci\skigen_ref.env"
set "SKIGEN_RELEASE_REF="
set "SKIGEN_DEV_REF="
if exist "%SKIGEN_REF_CONFIG%" (
    for /f "usebackq tokens=1,2 delims==" %%A in ("%SKIGEN_REF_CONFIG%") do (
        set "skigen_key=%%A"
        set "skigen_key=!skigen_key: =!"
        if not "!skigen_key!"=="" if not "!skigen_key:~0,1!"=="#" set "%%A=%%B"
    )
)
if defined SKIGEN_REF (
    set "SKIGEN_VERSION=%SKIGEN_REF%"
) else if defined SKIGEN_RELEASE_REF (
    set "SKIGEN_VERSION=%SKIGEN_RELEASE_REF%"
) else (
    set "SKIGEN_VERSION=v1.1.0"
)
set "SKIGEN_DIR=%SCRIPT_DIR%\skigen"
if not exist "%SKIGEN_DIR%" (
    echo Cloning skigen ^(!SKIGEN_VERSION!^)...
    git clone --branch !SKIGEN_VERSION! https://github.com/skigen-project/skigen.git "%SKIGEN_DIR%"
) else (
    echo Checking out skigen !SKIGEN_VERSION!...
    git -C "%SKIGEN_DIR%" fetch --tags origin 2>nul
    git -C "%SKIGEN_DIR%" checkout !SKIGEN_VERSION! 2>nul
)
echo   Skigen: %SKIGEN_DIR% ^(!SKIGEN_VERSION!^)

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
echo   --qt-version ^<version^>        Qt version to download ^(default: 6.11.1^)
echo   --eigen-version ^<version^>     Eigen version to download ^(default: 5.0.1^)
echo   --linkage ^<dynamic^|static^>   Qt linkage to prepare ^(default: dynamic^)
echo   --qt-dir ^<path^>               Target directory for the Qt bundle
echo   --eigen-dir ^<path^>            Target directory for the Eigen bundle
echo   --onnxruntime                 Enable ONNX Runtime using the central default version
echo   --onnxruntime-version ^<ver^>   ONNX Runtime version to download ^(default: none^)
echo   --onnxruntime-dir ^<path^>      Target directory for the ONNX Runtime package
echo   --onnxruntime-release-tag ^<t^> Override the ONNX Runtime prerelease tag
echo   --skip-qt                       Skip Qt download and keep using the provided/local Qt prefix
echo   --repository ^<owner/repo^>     Repository hosting the prerelease assets ^(default: mne-tools/mne-cpp^)
echo   --qt-release-tag ^<tag^>        Override the Qt prerelease tag
echo   --eigen-release-tag ^<tag^>     Override the Eigen prerelease tag
echo   --force                         Re-download even if the expected package already exists
echo   --help                          Show this help text
exit /b 0
