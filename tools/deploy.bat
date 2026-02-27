:;# This script performs generates and copies the necesary library dependencies for running qt-projects both for 
:;# dynamic and for staic builds. 
:;#
:;# This file is part of the MNE-CPP project. For more information visit: https://mne-cpp.github.io/
:;#
:;# This script is based on an open-source cross-platform script template.
:;# For more information you can visit: https://github.com/juangpc/multiplatform_bash_cmd
:;#

:<<BATCH
    @echo off
    :; # ####################################################
    :; # ########## WINDOWS SECTION #########################

    REM setlocal EnableDelayedExpansion

    SET SCRIPT_PATH=%~dp0
    SET BASE_PATH=%SCRIPT_PATH%..
    SET CURRENT_PATH=%cd%
    
    REM SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\"
    SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\"

    SET "LINK_OPTION=dynamic"
    SET "PACK_OPTION=false"
    SET "BUILD_NAME=Release"
    SET "MOCK_BUILD=false"
    SET "MOCK_TEXT="

    :loop
    IF NOT "%1"=="" (
        IF "%1"=="dynamic" (
            SET "LINK_OPTION=dynamic"
        )
        IF "%1"=="static" (
            SET "LINK_OPTION=static"
        )   
        IF "%1"=="pack" (
            SET "PACK_OPTION=true"
        )   
        IF "%1"=="help" (
            GOTO :showHelp
            GOTO :endOfScript
        )   
        IF "%1"=="mock" (
            SET "MOCK_BUILD=true"
        )
        IF "%1"=="build-name" (
            IF NOT "%2"=="" (
                SET "BUILD_NAME=%2"
                SHIFT
            )
        )
        SHIFT
        GOTO :loop
    )

    SET "OUT_DIR_NAME=%BASE_PATH%\out\%BUILD_NAME%"

    IF "%MOCK_BUILD%"=="true" (
        ECHO.
        ECHO Mock mode ON. Commands to be executed: 
        ECHO.
        SET "MOCK_TEXT=ECHO "
    )

    call :doPrintConfiguration
    
    IF "%LINK_OPTION%"=="dynamic" (
        
        cd %OUT_DIR_NAME%\apps

        REM Solve dependencies for top-level DLLs only (not plugin subdirs).
        REM Using /b without /s to avoid recursing into mne_scan_plugins/ etc.
        REM which would bloat each plugin dir with duplicate Qt DLLs.
        for /f %%f in ('dir /b *.dll') do (
          %MOCK_TEXT%windeployqt %%f
        )

        REM Solve dependencies for top-level applications only
        for /f %%f in ('dir /b *.exe') do (
            %MOCK_TEXT%windeployqt %%f
        )

        REM REM solve dependencies for tests 
        REM for /f %%f in ('dir test_*.exe /s /b') do (
        REM     %MOCK_TEXT%windeployqt %%f
        REM )
        REM
        REM REM solve dependencies for examples 
        REM for /f %%f in ('dir ex_*.exe /s /b') do (
        REM     %MOCK_TEXT%windeployqt %%f
        REM )

        IF "%PACK_OPTION%"=="true" (
            cd %BASE_PATH%
            REM Delete folders which we do not want to ship
            %MOCK_TEXT%rmdir %OUT_DIR_NAME%\resources\data /s /q 
            REM Creating archive from inside output directory for clean top-level layout
            cd %OUT_DIR_NAME%
            %MOCK_TEXT%7z a %BASE_PATH%\mne-cpp-windows-dynamic-x86_64.zip apps lib resources
            cd %BASE_PATH%
        )

        cd %cd%

    ) ELSE IF "%LINK_OPTION%"=="static" (
        
        IF "%PACK_OPTION%"=="true" (
            REM This script needs to be run from the top level mne-cpp repo folder
            REM Delete folders which we do not want to ship
            %MOCK_TEXT%rmdir %OUT_DIR_NAME%\resources\data /s /q
            %MOCK_TEXT%rmdir %OUT_DIR_NAME%\apps\mne_rt_server_plugins /s /q
            %MOCK_TEXT%rmdir %OUT_DIR_NAME%\apps\mne_scan_plugins /s /q
            %MOCK_TEXT%rmdir %OUT_DIR_NAME%\apps\mne_analyze_plugins /s /q
            
            REM Creating archive of everything in the bin directory
            %MOCK_TEXT%7z a %BASE_PATH%\mne-cpp-windows-static-x86_64.zip %OUT_DIR_NAME%
        )
        
    ) ELSE (
        ECHO Your link option: %LINK_OPTION% is not defined.
        goto :showHelp
        goto :endOfScript
    )
    
    :endOfScript

    exit /B

    :showHelp
      ECHO. 
      ECHO MNE-CPP Deplyment script help.
      ECHO. 
      ECHO Usage: ./deploy.bat [Options]
      ECHO.
      ECHO [help]  - Print this help.
      ECHO [dynamic/static] - Set the link type as dynamic (default) or static.
      ECHO [pack] - Enable output packaging into a compressed file.
      ECHO [build-name=]  - Specify the name of the build to deploy and 
      ECHO                         (if specified) pack.
      ECHO.
    exit /B 0

    :doPrintConfiguration
      ECHO.
      ECHO ====================================================================
      ECHO ===================== MNE-CPP DEPLOY SCRIPT =========================
      ECHO.
      ECHO LINK_OPTION  = %LINK_OPTION%
      ECHO PACK_OPTION  = %PACK_OPTION%
      ECHO BUILD_NAME   = %BUILD_NAME%
      ECHO OUT_DIR_NAME = %OUT_DIR_NAME%
      ECHO MOCK_BUILD   = %MOCK_BUILD%
      ECHO MOCK_TEXT    = %MOCK_TEXT%
      ECHO.
      ECHO ====================================================================
      ECHO ====================================================================
      ECHO.
    exit /B 0

    cd %CD%

    :; # ########## WINDOWS SECTION ENDS ####################
    :; #####################################################
    exit /b
BATCH

if [ "$(uname)" == "Darwin" ]; then
    
    # ######################################################
    # ############## MAC SECTION ###########################

argc=$#
argv=("$@")

function cleanAbsPath()
{
    local  cleanAbsPathStr="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$cleanAbsPathStr"
}

EXIT_SUCCESS=0
EXIT_FAIL=1
ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BasePath="$(cleanAbsPath "$ScriptPath/..")"
OutDirName=""
LinkOption="dynamic"
PackOption=""
BuildName="Release"
MockDeploy="false"
MockText=""

doPrintConfiguration() {
  echo " "
  echo ========================================================================
  echo ======================== MNE-CPP BUILD CONFIG ==========================
  echo " "
  echo " ScriptPath = $ScriptPath"
  echo " BasePath   = $BasePath"
  echo " OutDirName = $OutDirName"
  echo " "
  echo " LinkOption = $LinkOption"
  echo " PackOption = $PackOption"
  echo " BuildName  = $BuildName"
  echo " MockDeploy = $MockDeploy"
  echo " MockText   = $MockText"
  echo " "
  echo ========================================================================
  echo ========================================================================
  echo " "
}

doPrintHelp() {
  echo " "
  echo "Usage: ./deploy.bat [Options]"
  echo " "
  echo "All options can be used in undefined order."
  echo " "
  echo "[help] - Print this help."
  echo "[mock] - Show commands do not execute them."
  echo "[dynamic/static] - Set the link type as dynamic (default) or static."
  echo "[pack] - Enable packaging of applications into a compressed file."
  echo "[build-name=] - Specify the name of the build to deploy."
  echo " "
}

for (( j=0; j<argc; j++ )); do
    if [ "${argv[j]}" == "dynamic" ]; then
        LinkOption="dynamic"
    elif [ "${argv[j]}" == "static" ]; then
        LinkOption="static"
    elif [ "${argv[j]}" == "pack" ]; then
        PackOption="true"
    elif [ "${argv[j]}" == "help" ]; then
        PrintHelp="true"
    elif [ "${argv[j]}" == "mock" ]; then
        MockBuild="true"
    fi
    IFS='=' read -r -a inkarg <<< "${argv[j]}"
    if [[ "${inkarg[0]}" == "build-name" ]]; then
        echo "ink"
        BuildName="${inkarg[1]}"
    fi
done

OutFolder=${BASE_PATH}/out/${BuildName}

if [ "${MockBuild}" == "true" ]; then
  MockText="echo "
  echo " "
  echo "Mock mode ON. Commands to be executed: "
  echo " "
else
  MockText=""
fi

if [ "${PrintHelp}" == "true" ]; then
    doPrintHelp
    exit ${EXIT_SUCCESS} 
fi

doPrintConfiguration

if [[ ${LinkOption} == "dynamic" ]]; then

    # ---------------------------------------------------------------
    # Locate macdeployqt — works with both Qt 5 ($Qt5_DIR) and Qt 6
    # ($QT_ROOT_DIR or PATH set by jurplel/install-qt-action).
    # ---------------------------------------------------------------
    MACDEPLOYQT=""
    if command -v macdeployqt &> /dev/null; then
        MACDEPLOYQT="macdeployqt"
    elif [ -n "${QT_ROOT_DIR}" ] && [ -x "${QT_ROOT_DIR}/bin/macdeployqt" ]; then
        MACDEPLOYQT="${QT_ROOT_DIR}/bin/macdeployqt"
    elif [ -n "${Qt6_DIR}" ] && [ -x "${Qt6_DIR}/../../../bin/macdeployqt" ]; then
        MACDEPLOYQT="${Qt6_DIR}/../../../bin/macdeployqt"
    elif [ -n "${Qt5_DIR}" ] && [ -x "${Qt5_DIR}/bin/macdeployqt" ]; then
        MACDEPLOYQT="${Qt5_DIR}/bin/macdeployqt"
    fi

    if [ -z "${MACDEPLOYQT}" ]; then
        echo "ERROR: macdeployqt not found. Ensure Qt bin/ is on PATH or set QT_ROOT_DIR."
        exit 1
    fi
    echo "Using macdeployqt: ${MACDEPLOYQT}"

    # Call macdeployqt on all .app bundles in the apps folder.
    # macdeployqt copies Qt frameworks for the main binary's direct dependencies
    # into Contents/Frameworks/. It does NOT resolve MNE-CPP dylib dependencies.
    for f in ${BasePath}/out/${BuildName}/apps/*.app; do
        echo "Running macdeployqt on $(basename $f) ..."
        ${MockText}${MACDEPLOYQT} "$f"
    done

    # ---------------------------------------------------------------
    # Locate Qt lib directory for framework copying
    # ---------------------------------------------------------------
    QT_LIB_DIR=""
    if [ -n "${QT_ROOT_DIR}" ] && [ -d "${QT_ROOT_DIR}/lib" ]; then
        QT_LIB_DIR="${QT_ROOT_DIR}/lib"
    elif [ -n "${Qt6_DIR}" ]; then
        QT_LIB_DIR="$(cd "${Qt6_DIR}/../../.." 2>/dev/null && pwd)/lib"
    elif [ -n "${Qt5_DIR}" ] && [ -d "${Qt5_DIR}/lib" ]; then
        QT_LIB_DIR="${Qt5_DIR}/lib"
    fi

    # ---------------------------------------------------------------
    # Collect ALL required Qt frameworks (from CLI tools, MNE-CPP dylibs,
    # and transitive Qt dependencies). macdeployqt only handles the main
    # .app binary's direct Qt deps; MNE-CPP dylibs pull in additional
    # Qt frameworks (Qt3D*, QtCharts, QtOpenGL, etc.) that we must add.
    # ---------------------------------------------------------------
    echo "Resolving all required Qt frameworks..."
    QT_FRAMEWORKS=""
    for exe in ${BasePath}/out/${BuildName}/apps/mne_*; do
        [ -f "$exe" ] && [ -x "$exe" ] || continue
        DEPS=$(otool -L "$exe" 2>/dev/null | grep -o '@rpath/Qt[^/]*\.framework' | sed 's|@rpath/||' | sort -u)
        QT_FRAMEWORKS="${QT_FRAMEWORKS} ${DEPS}"
    done
    for dylib in ${BasePath}/out/${BuildName}/lib/*.dylib; do
        DEPS=$(otool -L "$dylib" 2>/dev/null | grep -o '@rpath/Qt[^/]*\.framework' | sed 's|@rpath/||' | sort -u)
        QT_FRAMEWORKS="${QT_FRAMEWORKS} ${DEPS}"
    done
    # Resolve transitive Qt framework dependencies
    if [ -n "${QT_LIB_DIR}" ] && [ -d "${QT_LIB_DIR}" ]; then
        for fw in $(echo "${QT_FRAMEWORKS}" | tr ' ' '\n' | sort -u | grep -v '^$'); do
            FW_NAME=$(echo "$fw" | sed 's/\.framework//')
            if [ -f "${QT_LIB_DIR}/${fw}/Versions/A/${FW_NAME}" ]; then
                TRANS_DEPS=$(otool -L "${QT_LIB_DIR}/${fw}/Versions/A/${FW_NAME}" 2>/dev/null | grep -o '@rpath/Qt[^/]*\.framework' | sed 's|@rpath/||' | sort -u)
                QT_FRAMEWORKS="${QT_FRAMEWORKS} ${TRANS_DEPS}"
            fi
        done
    fi
    QT_FRAMEWORKS=$(echo "${QT_FRAMEWORKS}" | tr ' ' '\n' | sort -u | grep -v '^$')
    echo "  Required Qt frameworks: $(echo ${QT_FRAMEWORKS} | tr '\n' ' ')"

    # ---------------------------------------------------------------
    # Complete .app bundles: copy MNE-CPP dylibs + missing Qt frameworks
    # into each .app bundle's Contents/Frameworks/.
    # ---------------------------------------------------------------
    for f in ${BasePath}/out/${BuildName}/apps/*.app; do
        FWDIR="$f/Contents/Frameworks"
        [ -d "$FWDIR" ] || continue
        echo "Completing $(basename $f) bundle..."

        # Copy MNE-CPP shared libraries into Frameworks/
        for dylib in ${BasePath}/out/${BuildName}/lib/*.dylib; do
            LIBNAME=$(basename "$dylib")
            [ -f "${FWDIR}/${LIBNAME}" ] || ${MockText}cp -a "$dylib" "${FWDIR}/"
        done

        # Copy any missing Qt frameworks (needed by MNE-CPP dylibs)
        if [ -n "${QT_LIB_DIR}" ]; then
            for fw in ${QT_FRAMEWORKS}; do
                if [ ! -d "${FWDIR}/${fw}" ] && [ -d "${QT_LIB_DIR}/${fw}" ]; then
                    ${MockText}cp -a "${QT_LIB_DIR}/${fw}" "${FWDIR}/"
                fi
            done
        fi
    done

    # Copy plugins and resources into the relevant .app bundles.

    # mne_scan.app — plugins + resources
    if [ -d "${BasePath}/out/${BuildName}/apps/mne_scan.app" ]; then
        ${MockText}cp -a ${BasePath}/out/${BuildName}/apps/mne_scan_plugins ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/MacOS/mne_scan_plugins
        ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/MacOS/resources
    fi

    # mne_analyze.app — plugins + resources
    if [ -d "${BasePath}/out/${BuildName}/apps/mne_analyze.app" ]; then
        ${MockText}cp -a ${BasePath}/out/${BuildName}/apps/mne_analyze_plugins ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/MacOS/mne_analyze_plugins
        ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/MacOS/resources
    fi

    # mne_rt_server.app — plugins + resources (if it exists as a .app bundle)
    if [ -d "${BasePath}/out/${BuildName}/apps/mne_rt_server.app" ]; then
        ${MockText}cp -a ${BasePath}/out/${BuildName}/apps/mne_rt_server_plugins ${BasePath}/out/${BuildName}/apps/mne_rt_server.app/Contents/MacOS/mne_rt_server_plugins
        ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_rt_server.app/Contents/MacOS/resources
    fi

    # ---------------------------------------------------------------
    # Copy Qt frameworks into lib/ for standalone CLI executables.
    # CLI tools use @rpath=@executable_path/../lib to find dependencies.
    # ---------------------------------------------------------------
    echo "Copying Qt frameworks into lib/ for CLI tools..."
    if [ -n "${QT_LIB_DIR}" ] && [ -d "${QT_LIB_DIR}" ]; then
        for fw in ${QT_FRAMEWORKS}; do
            if [ -d "${QT_LIB_DIR}/${fw}" ]; then
                echo "  Copying ${fw} to lib/ ..."
                ${MockText}cp -a "${QT_LIB_DIR}/${fw}" "${BasePath}/out/${BuildName}/lib/"
            else
                echo "  WARNING: ${fw} not found in ${QT_LIB_DIR}"
            fi
        done
    else
        echo "  WARNING: Could not find Qt lib directory. CLI tools may not find Qt at runtime."
    fi

    if [[ ${PackOption} == "true" ]]; then

        # Delete folders which we do not want to ship
        ${MockText}rm -rf ${BasePath}/out/${BuildName}/resources/data
        # mne_scan and mne_analyze plugins are inside their .app bundles now
        ${MockText}rm -rf ${BasePath}/out/${BuildName}/apps/mne_scan_plugins
        ${MockText}rm -rf ${BasePath}/out/${BuildName}/apps/mne_analyze_plugins
        # mne_rt_server_plugins stays — mne_rt_server is a CLI tool (no .app bundle)

        # Creating archive of all macos deployed applications.
        # Include apps/ (with Qt frameworks inside .app bundles), lib/
        # (MNE-CPP shared libraries + Qt frameworks for CLI tools),
        # and resources/ (config files needed by CLI tools like mne_rt_server).
        ${MockText}tar cfvz mne-cpp-macos-dynamic-arm64.tar.gz -C ${BasePath}/out/${BuildName} apps lib resources
    fi

elif [[ ${LinkOption} == "static" ]]; then

    # This script needs to be run from the top level mne-cpp repo folder
    # Solve for dependencies for mne_scan.app bundle
    ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/MacOS/resources

    # Solve for dependencies for mne_analyze.app bundle
    ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/MacOS/resources

    # Solve for dependencies for mne_rt_server.app bundle
    ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_rt_server.app/Contents/MacOS/resources

    # Solve for dependencies for mne_forward_solution.app bundle
    ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_forward_solution.app/Contents/MacOS/resources

    # Solve for dependencies for mne_dipole_fit.app bundle
    ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_dipole_fit.app/Contents/MacOS/resources 

    # Solve for dependencies for mne_anonymize.app bundle
    ${MockText}cp -a ${BasePath}/out/${BuildName}/resources ${BasePath}/out/${BuildName}/apps/mne_anonymize.app/Contents/MacOS/resources

    if [[ ${PackOption} == "true" ]]; then
        # Delete folders which we do not want to ship
        ${MockText}cp -r ${BasePath}/out/${BuildName}/ mne-cpp
        ${MockText}rm -r mne-cpp/resources/data
        ${MockText}rm -r mne-cpp/apps/mne_scan_plugins
        ${MockText}rm -r mne-cpp/apps/mne_analyze_plugins
        ${MockText}rm -r mne-cpp/apps/mne_rt_server_plugins

        # Creating archive of all macos deployed applications
        ${MockText}tar cfvz mne-cpp-macos-static-arm64.tar.gz mne-cpp
    fi

else 
    echo "Input argument link_option is invalid."
    doPrintConfiguration
    doPrintHelp
    exit ${EXIT_FAIL}
fi

    # ############## MAC SECTION ENDS ######################
    # ######################################################

elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    
    # ######################################################
    # ############## LINUX SECTION #########################


argc=$#
argv=("$@")

function cleanAbsPath()
{
    local  cleanAbsPathStr="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$cleanAbsPathStr"
}


EXIT_FAIL=1
EXIT_SUCCESS=0
ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BasePath="$(cleanAbsPath "$ScriptPath/..")"
OutDirName=""
LinkOption="dynamic"
PackOption=""
BuildName=""
MockDeploy="false"
MockText=""
MartinosOption="false"

doPrintConfiguration() {
  echo " "
  echo ========================================================================
  echo ======================== MNE-CPP BUILD CONFIG ==========================
  echo " "
  echo " ScriptPath = $ScriptPath"
  echo " BasePath   = $BasePath"
  echo " OutDirName = $OutDirName"
  echo " "
  echo " LinkOption = $LinkOption"
  echo " PackOption = $PackOption"
  echo " BuildName  = $BuildName"
  echo " MockDeploy = $MockDeploy"
  echo " MockText   = $MockText"
  echo " Martinos   = $MartinosOption"
  echo " "
  echo ========================================================================
  echo ========================================================================
  echo " "
}

doPrintHelp() {
  echo " "
  echo "Usage: ./deploy.bat [Options]"
  echo " "
  echo "All options can be used in undefined order."
  echo " "
  echo "[help] - Print this help."
  echo "[mock] - Show commands do not execute them."
  echo "[dynamic/static] - Set the link type as dynamic (default) or static."
  echo "[pack] - Enable packaging of applications into a compressed file."
  echo "[build-name=] - Specify the name of the build to deploy."
  echo "[martinos] - Specify if the installation of for Martinos Center Centos."
  echo " "
}

for (( j=0; j<argc; j++ )); do
    if [ "${argv[j]}" == "dynamic" ]; then
        LinkOption="dynamic"
    elif [ "${argv[j]}" == "static" ]; then
        LinkOption="static"
    elif [ "${argv[j]}" == "pack" ]; then
        PackOption="true"
    elif [ "${argv[j]}" == "help" ]; then
        PrintHelp="true"
    elif [ "${argv[j]}" == "mock" ]; then
        MockBuild="true"
    elif [ "${argv[j]}" == "martinos" ]; then
        MartinosOption="true"
    fi
    IFS='=' read -r -a inkarg <<< "${argv[j]}"
    if [ "${inkarg[0]}" == "build-name" ]; then
        BuildName="${inkarg[1]}"
    fi
done

OutFolder=${BasePath}/out/${BuildName}

if [ "${MockBuild}" == "true" ]; then
  MockText="echo "
  echo " "
  echo "Mock mode ON. Commands to be executed: "
  echo " "
else
  MockText=""
fi
    
if [ "${PrintHelp}" == "true" ]; then
    doPrintHelp
    exit ${EXIT_SUCCESS} 
fi

doPrintConfiguration

if [[ ${LinkOption} == "dynamic" ]]; then

    if [[ ${MartinosOption} == "true"  ]]; then
        if [[ -z "${Qt_ROOT_FOLDER}" ]]; then
            echo "You need to define the variable: Qt_ROOT_FOLDER. And make it"
            echo "point to your Qt installation."
        else
            cp -r ${Qt_ROOT_FOLDER}/plugins/platforms ${BASE_PATH}/out/${BUILD_NAME}/apps
            cp -r ${Qt_ROOT_FOLDER}/plugins/xcbglintegrations ${BASE_PATH}/out/${BUILD_NAME}/apps
            cp -r ${Qt_ROOT_FOLDER}/lib/*.so ${BASE_PATH}/out/${BUILD_NAME}/lib
        fi
    else
        # Install some additional packages for xcb platform plugin
        sudo apt-get update
        sudo apt-get install -y libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 \
            libxcb-keysyms1 libxcb-render-util0 libbluetooth3 libxcb-xinerama0 \
            libxcb-cursor0
        ${MockText}export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BasePath}/out/${BuildName}/lib

        # ---------------------------------------------------------------
        # Locate the Qt installation directory.
        # jurplel/install-qt-action sets Qt6_DIR to either:
        #   - the Qt prefix     (e.g. <root>/Qt/6.x.y/gcc_64)      → has lib/
        #   - the CMake cfg dir (e.g. <prefix>/lib/cmake/Qt6)       → go up 3
        # QT_ROOT_DIR always takes precedence if set explicitly.
        # ---------------------------------------------------------------
        QT_DIR=""
        if [ -n "${QT_ROOT_DIR}" ]; then
            QT_DIR="${QT_ROOT_DIR}"
        elif [ -n "${Qt6_DIR}" ]; then
            if [ -d "${Qt6_DIR}/lib" ]; then
                # Qt6_DIR is already the prefix directory
                QT_DIR="${Qt6_DIR}"
            else
                # Qt6_DIR is the CMake config dir; go up 3 levels
                QT_DIR="$(cd "${Qt6_DIR}/../../.." && pwd)"
            fi
        elif [ -n "${Qt5_DIR}" ]; then
            QT_DIR="${Qt5_DIR}"
        fi

        if [ -z "${QT_DIR}" ] || [ ! -d "${QT_DIR}/lib" ]; then
            echo "ERROR: Cannot locate Qt installation. Set QT_ROOT_DIR."
            echo "  QT_ROOT_DIR=${QT_ROOT_DIR:-<not set>}"
            echo "  Qt6_DIR=${Qt6_DIR:-<not set>}"
            echo "  Qt5_DIR=${Qt5_DIR:-<not set>}"
            exit 1
        fi
        echo "Using Qt from: ${QT_DIR}"

        # Copy Qt shared libraries into lib/
        echo "Copying Qt shared libraries..."
        ${MockText}find "${QT_DIR}/lib" -maxdepth 1 -name "libQt6*.so*" -exec cp -a {} ${BasePath}/out/${BuildName}/lib/ \;
        ${MockText}find "${QT_DIR}/lib" -maxdepth 1 -name "libicu*.so*" -exec cp -a {} ${BasePath}/out/${BuildName}/lib/ \;

        # Copy Qt plugins (platforms, imageformats, etc.)
        echo "Copying Qt plugins..."
        ${MockText}mkdir -p ${BasePath}/out/${BuildName}/plugins
        for plugdir in platforms imageformats xcbglintegrations wayland-shell-integration; do
            if [ -d "${QT_DIR}/plugins/${plugdir}" ]; then
                ${MockText}cp -a "${QT_DIR}/plugins/${plugdir}" ${BasePath}/out/${BuildName}/plugins/
            fi
        done

        # Manually copy in the libxcb-xinerama library which is needed by plugins/platforms/libqxcb.so
        XCBXINERAMA="/usr/lib/x86_64-linux-gnu/libxcb-xinerama.so.0"
        if [ -f "${XCBXINERAMA}" ]; then
            ${MockText}cp "${XCBXINERAMA}" ${BasePath}/out/${BuildName}/lib/
        fi

        if [[ ${PackOption} == "true" ]]; then
            echo
            echo ldd ${BasePath}/out/${BuildName}/apps/mne_scan
            ${MockText}ldd ${BasePath}/out/${BuildName}/apps/mne_scan

            # Delete folders which we do not want to ship
            ${MockText}cp -r ${BasePath}/out/${BuildName}/ mne-cpp
            ${MockText}rm -rf mne-cpp/resources/data

            # Creating archive of everything in current directory
            ${MockText}tar cfvz ${BasePath}/mne-cpp-linux-dynamic-x86_64.tar.gz mne-cpp
        fi
    fi

elif [[ ${LinkOption} == "static" ]]; then
    if [[ ${MartinosOption} == "true"  ]]; then
        echo "OoOps!"
        echo "Martinos Option not supported in static mode."
        echo " "
        exit 0
    fi
    sudo apt-get update
    sudo apt-get install -y libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 \
        libxcb-keysyms1 libxcb-render-util0 libbluetooth3 libxcb-xinerama0 \
        libxcb-cursor0
    ${MockText}export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BasePath}/out/${BuildName}/lib

    # Static builds don't need Qt deploy tools — Qt is linked in.
    # Just package the output.

    # Delete folders which we do not want to ship
    ${MockText}cp -r ${BasePath}/out/${BuildName}/ mne-cpp
    ${MockText}rm -rf mne-cpp/resources/data
    ${MockText}rm -rf mne-cpp/apps/mne_rt_server_plugins
    ${MockText}rm -rf mne-cpp/apps/mne_scan_plugins
    ${MockText}rm -rf mne-cpp/apps/mne_analyze_plugins

    if [[ ${PackOption} == "true" ]]; then
        # Creating archive of everything in the bin directory
        ${MockText}tar cfvz ${BasePath}/mne-cpp-linux-static-x86_64.tar.gz mne-cpp
    fi

else 
    echo "Error. Link option can only be dynamic or static."
    doPrintHelp
    exit ${EXIT_FAIL}
fi

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0

