@echo off
setlocal EnableDelayedExpansion

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "QT_VERSION=6.11.0"
set "EIGEN_VERSION=5.0.1"
set "LINKAGE=dynamic"
set "BUILD_TYPE=Release"
set "BUILD_DIR="
set "QT_DIR="
set "QT_MODE=auto"
set "EIGEN_DIR="
set "REPOSITORY=mne-tools/mne-cpp"
set "QT_RELEASE_TAG="
set "EIGEN_RELEASE_TAG="
set "DEPS_ONLY=0"
set "FORCE=0"
set "QT_DIR_EXPLICIT=0"
set "NO_OPENGL_VALUE=ON"
set "EXTRA_MODE=0"
set "EXTRA_CMAKE_ARGS="
set "QT_SOURCE="
set "QT_ARTIFACT_DIR="
set "QT_CANDIDATE_COUNT=0"
set "PROBED_QT_ROOT_DIR="
set "QT_PROBE_LAST_DIR="
set "QT_PROBE_LAST_LOG="

:parse_args
if "%~1"=="" goto after_parse
if "%EXTRA_MODE%"=="1" (
    set "EXTRA_CMAKE_ARGS=!EXTRA_CMAKE_ARGS! %1"
    shift
    goto parse_args
)
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
if /I "%~1"=="--build-type" (
    set "BUILD_TYPE=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--build-dir" (
    set "BUILD_DIR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--qt-dir" (
    set "QT_DIR=%~2"
    set "QT_DIR_EXPLICIT=1"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--qt-mode" (
    set "QT_MODE=%~2"
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
if /I "%~1"=="--with-opengl" (
    set "NO_OPENGL_VALUE=OFF"
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
if /I "%~1"=="--deps-only" (
    set "DEPS_ONLY=1"
    shift
    goto parse_args
)
if /I "%~1"=="--force" (
    set "FORCE=1"
    shift
    goto parse_args
)
if /I "%~1"=="--" (
    set "EXTRA_MODE=1"
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

if /I not "%QT_MODE%"=="auto" if /I not "%QT_MODE%"=="system" if /I not "%QT_MODE%"=="artifact" (
    echo --qt-mode must be one of "auto", "system", or "artifact" 1>&2
    exit /b 1
)

if not defined EIGEN_DIR set "EIGEN_DIR=%REPO_ROOT%\src\external\eigen"
if not defined BUILD_DIR set "BUILD_DIR=%REPO_ROOT%\build\developer-%LINKAGE%"
set "QT_ARTIFACT_DIR=%REPO_ROOT%\src\external\qt\%LINKAGE%"

echo %EXTRA_CMAKE_ARGS% | findstr /I /C:"-DNO_OPENGL=OFF" /C:"-DNO_OPENGL:BOOL=OFF" >nul && set "NO_OPENGL_VALUE=OFF"
echo %EXTRA_CMAKE_ARGS% | findstr /I /C:"-DNO_OPENGL=ON" /C:"-DNO_OPENGL:BOOL=ON" >nul && set "NO_OPENGL_VALUE=ON"

call :resolve_qt_dir
if errorlevel 1 exit /b 1

set "CMAKE_PREFIX_VALUE=%QT_DIR%"
if exist "%EIGEN_DIR%\share\eigen3\cmake\Eigen3Config.cmake" (
    set "CMAKE_PREFIX_VALUE=%QT_DIR%;%EIGEN_DIR%"
)

set "FORCE_ARG="
if "%FORCE%"=="1" set "FORCE_ARG=--force"

if /I "%QT_SOURCE%"=="artifact" (
    call "%REPO_ROOT%\src\external\init.bat" --qt-version "%QT_VERSION%" --eigen-version "%EIGEN_VERSION%" --linkage "%LINKAGE%" --qt-dir "%QT_DIR%" --eigen-dir "%EIGEN_DIR%" --repository "%REPOSITORY%" --qt-release-tag "%QT_RELEASE_TAG%" --eigen-release-tag "%EIGEN_RELEASE_TAG%" %FORCE_ARG%
) else (
    call "%REPO_ROOT%\src\external\init.bat" --qt-version "%QT_VERSION%" --eigen-version "%EIGEN_VERSION%" --linkage "%LINKAGE%" --qt-dir "%QT_DIR%" --skip-qt --eigen-dir "%EIGEN_DIR%" --repository "%REPOSITORY%" --qt-release-tag "%QT_RELEASE_TAG%" --eigen-release-tag "%EIGEN_RELEASE_TAG%" %FORCE_ARG%
)
if errorlevel 1 exit /b 1

if "%DEPS_ONLY%"=="1" (
    echo.
    echo Dependencies are ready in src\external.
    exit /b 0
)

set "STATIC_ARG="
if /I "%LINKAGE%"=="static" set "STATIC_ARG=-DBUILD_SHARED_LIBS=OFF"

cmake -B "%BUILD_DIR%" -S "%REPO_ROOT%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DNO_OPENGL=%NO_OPENGL_VALUE% "-DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_VALUE%" %STATIC_ARG% %EXTRA_CMAKE_ARGS%
if errorlevel 1 exit /b 1

echo.
echo Developer configure complete.
echo   Build directory: %BUILD_DIR%
echo   Qt source: %QT_SOURCE% ^(%QT_DIR%^)
echo   NO_OPENGL: %NO_OPENGL_VALUE%
echo   Next step: cmake --build "%BUILD_DIR%" --parallel
exit /b 0

:usage_ok
call :usage
exit /b 0

:usage_fail
call :usage
exit /b 1

:usage
echo Usage: init.bat [options] [-- ^<extra cmake args^>]
echo.
echo Bootstraps a developer build by downloading the MNE-CPP-maintained Qt and Eigen
echo artifacts into src\external and configuring a build directory from the repo root.
echo.
echo Options:
echo   --qt-version ^<version^>        Qt version to use ^(default: 6.11.0^)
echo   --eigen-version ^<version^>     Eigen version to use ^(default: 5.0.1^)
echo   --linkage ^<dynamic^|static^>   Qt linkage / MNE-CPP linkage ^(default: dynamic^)
echo   --build-type ^<type^>           CMake build type ^(default: Release^)
echo   --build-dir ^<path^>            Build directory ^(default: build\developer-^<linkage^>^)
echo   --qt-dir ^<path^>               Use this existing Qt prefix after validating compatibility
echo   --qt-mode ^<auto^|system^|artifact^>
echo                                     auto: prefer compatible local Qt, otherwise download artifact
echo                                     system: require compatible local Qt
echo                                     artifact: always download/use the MNE-CPP Qt artifact
echo   --eigen-dir ^<path^>            Use an existing Eigen package instead of downloading one
echo   --with-opengl                   Configure with QOpenGLWidget support instead of the default QRhi-only path
echo   --repository ^<owner/repo^>     Repository hosting the prerelease assets ^(default: mne-tools/mne-cpp^)
echo   --qt-release-tag ^<tag^>        Override the Qt prerelease tag
echo   --eigen-release-tag ^<tag^>     Override the Eigen prerelease tag
echo   --deps-only                     Only prepare src\external, do not run CMake configure
echo   --force                         Re-download dependency bundles even if present
echo   --help                          Show this help text
exit /b 0

:add_qt_candidate
set "CANDIDATE=%~1"
if not defined CANDIDATE exit /b 0
if exist "%CANDIDATE%" (
    if not exist "%CANDIDATE%\*" (
        for %%I in ("%CANDIDATE%") do set "CANDIDATE=%%~dpI"
    )
)
if not exist "%CANDIDATE%" exit /b 0
for %%I in ("%CANDIDATE%") do set "CANDIDATE=%%~fI"
if "%CANDIDATE:~-1%"=="\" set "CANDIDATE=%CANDIDATE:~0,-1%"
for /L %%N in (1,1,%QT_CANDIDATE_COUNT%) do (
    if /I "!QT_CANDIDATE_%%N!"=="%CANDIDATE%" exit /b 0
)
set /a QT_CANDIDATE_COUNT+=1
set "QT_CANDIDATE_%QT_CANDIDATE_COUNT%=%CANDIDATE%"
exit /b 0

:add_qt_candidate_from_query_tool
where %~1 >nul 2>&1 || exit /b 0
for /f "delims=" %%I in ('where %~1 2^>nul') do (
    for /f "usebackq delims=" %%J in (`"%%~I" -query QT_INSTALL_PREFIX 2^>nul`) do (
        call :add_qt_candidate "%%~J"
        exit /b 0
    )
    for %%J in ("%%~dpI..") do (
        call :add_qt_candidate "%%~fJ"
        exit /b 0
    )
)
exit /b 0

:collect_qt_candidates
call :add_qt_candidate "%QT_ROOT_DIR%"
call :add_qt_candidate "%QTDIR%"
call :add_qt_candidate "%Qt6_DIR%"
call :add_qt_candidate_from_query_tool qmake6
call :add_qt_candidate_from_query_tool qmake
call :add_qt_candidate_from_query_tool qtpaths6
call :add_qt_candidate_from_query_tool qtpaths
call :add_qt_candidate_from_query_tool qt-cmake
call :add_qt_candidate "%USERPROFILE%\Qt\%QT_VERSION%\msvc2022_64"
call :add_qt_candidate "%USERPROFILE%\Qt\%QT_VERSION%\msvc2019_64"
call :add_qt_candidate "%USERPROFILE%\Qt\%QT_VERSION%\mingw_64"
call :add_qt_candidate "%SystemDrive%\Qt\%QT_VERSION%\msvc2022_64"
call :add_qt_candidate "%SystemDrive%\Qt\%QT_VERSION%\msvc2019_64"
call :add_qt_candidate "%SystemDrive%\Qt\%QT_VERSION%\mingw_64"
exit /b 0

:probe_qt_candidate
set "PROBED_QT_ROOT_DIR="
set "QT_PROBE_LAST_DIR=%TEMP%\mnecpp-qt-probe-%RANDOM%%RANDOM%%RANDOM%"
set "QT_PROBE_LAST_LOG=%QT_PROBE_LAST_DIR%\probe.log"
set "QT_PROBE_RESULT=%QT_PROBE_LAST_DIR%\result.txt"
mkdir "%QT_PROBE_LAST_DIR%" >nul 2>&1 || exit /b 1
cmake -S "%REPO_ROOT%\cmake\qt_probe" -B "%QT_PROBE_LAST_DIR%\build" -DMNE_QT_PROBE_CANDIDATE:PATH=%~1 -DMNE_QT_PROBE_VERSION:STRING=%QT_VERSION% -DMNE_QT_PROBE_LINKAGE:STRING=%LINKAGE% -DMNE_QT_PROBE_NO_OPENGL:BOOL=%NO_OPENGL_VALUE% -DMNE_QT_PROBE_RESULT_FILE:FILEPATH=%QT_PROBE_RESULT% > "%QT_PROBE_LAST_LOG%" 2>&1
if errorlevel 1 exit /b 1
for /f "usebackq tokens=1* delims==" %%A in ("%QT_PROBE_RESULT%") do (
    if /I "%%A"=="QT_ROOT_DIR" set "PROBED_QT_ROOT_DIR=%%B"
)
if not defined PROBED_QT_ROOT_DIR exit /b 1
exit /b 0

:cleanup_qt_probe
if defined QT_PROBE_LAST_DIR if exist "%QT_PROBE_LAST_DIR%" rmdir /s /q "%QT_PROBE_LAST_DIR%"
set "QT_PROBE_LAST_DIR="
set "QT_PROBE_LAST_LOG="
set "QT_PROBE_RESULT="
exit /b 0

:resolve_qt_dir
if "%QT_DIR_EXPLICIT%"=="1" (
    call :probe_qt_candidate "%QT_DIR%"
    if errorlevel 1 (
        echo The Qt prefix provided via --qt-dir is not compatible with MNE-CPP. 1>&2
        echo Expected Qt %QT_VERSION% ^(%LINKAGE%^), NO_OPENGL=%NO_OPENGL_VALUE%. 1>&2
        if defined QT_PROBE_LAST_LOG if exist "%QT_PROBE_LAST_LOG%" type "%QT_PROBE_LAST_LOG%" 1>&2
        call :cleanup_qt_probe
        exit /b 1
    )
    set "QT_DIR=%PROBED_QT_ROOT_DIR%"
    set "QT_SOURCE=local"
    call :cleanup_qt_probe
    exit /b 0
)

if /I not "%QT_MODE%"=="artifact" (
    call :collect_qt_candidates
    for /L %%N in (1,1,%QT_CANDIDATE_COUNT%) do (
        call :probe_qt_candidate "!QT_CANDIDATE_%%N!"
        if not errorlevel 1 (
            set "QT_DIR=!PROBED_QT_ROOT_DIR!"
            set "QT_SOURCE=local"
            call :cleanup_qt_probe
            exit /b 0
        )
        call :cleanup_qt_probe
    )
    if /I "%QT_MODE%"=="system" (
        echo Could not find a compatible local Qt %QT_VERSION% for a %LINKAGE% build. 1>&2
        exit /b 1
    )
)

set "QT_DIR=%QT_ARTIFACT_DIR%"
set "QT_SOURCE=artifact"
exit /b 0
