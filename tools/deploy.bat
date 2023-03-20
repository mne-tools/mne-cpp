:;# This script performs generates and copies the necesary library dependencies for running qt-projects both for 
:;# dynamic and for staic builds. 
:;#
:;# This file is part of the MNE-CPP project. For more information visit: https://mne-cpp.github.io/
:;#
:;# This script is based on an open-source cross-platform script template.
:;# For more information you can visit: https://github.com/juangpc/multiplatform_bash_cmd
:;#

:<<BATCH
    :;@echo off
    :; # ####################################################
    :; # ########## WINDOWS SECTION #########################

    SET SCRIPT_PATH=%~dp0
    SET BASE_PATH=%SCRIPT_PATH%..
    SET CURRENT_PATH=
    
    SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\"

    SET "LINK_OPTION=dynamic"
    SET "PACK_OPTION="
    SET "BUILD_NAME="
    SET "MOCK_BUILD=False"
    SET "MOCK_TEXT="

    :loop
    IF NOT "%1"=="" (
        IF "%1"=="dynamic" (
            SET "LINK_OPTION=dynamic"
        )
        IF "%1"=="static" (
            SET "LINK_OPTION=dynamic"
        )   
        IF "%1"=="pack" (
            SET "PACK_OPTION=True"
        )   
        IF "%1"=="help" (
            GOTO :showHelp
            GOTO :endOfScript
        )   
        IF "%1"=="mock" (
            SET "MOCK_BUILD=True"
        )
        for /F "tokens=1 delims==" %%a in ("%1") do (
            REM ECHO %%a
            REM ECHO %%b
            IF "%%a"=="build-name" (
                SET "BUILD_NAME"=="%%b"
            )
        ) 
        SHIFT
        GOTO :loop
    )

    SET "OUT_DIR_NAME=%BASE_PATH%\out\%BUILD_NAME%"

    IF "%MOCK_BUILD%"=="True" (
        ECHO.
        ECHO Mock mode ON. Commands to be executed: 
        ECHO.
        SET "MOCK_TEXT=ECHO "
    )

    call :doPrintConfiguration

    IF "%LINK_OPTION%"=="dynamic" (
        
        cd %OUT_DIR_NAME%\apps

        REM Solve dependencies for libraries
        for /f %%f in ('dir *.dll /s /b') do (
          %MOCK_TEXT%windeployqt %%f
        )

        REM solve dependencies for applications
        for /f %%f in ('dir *.exe /s /b') do (
            %MOCK_TEXT%windeployqt %%f
        )

        REM solve dependencies for tests 
        for /f %%f in ('dir test_*.exe /s /b') do (
            %MOCK_TEXT%windeployqt %%f
        )

        REM solve dependencies for examples 
        for /f %%f in ('dir ex_*.exe /s /b') do (
            %MOCK_TEXT%windeployqt %%f
        )

        IF "%PACK_OPTION%"=="pack" (
            cd %BASE_PATH%
            REM Delete folders which we do not want to ship
            %MOCK_TEXT%rmdir %OUT_DIR_NAME%\resources\data /s /q 
            REM Creating archive of all win deployed applications
            %MOCK_TEXT%7z a %BASE_PATH%\mne-cpp-windows-dynamic-x86_64.zip %OUT_DIR_NAME%
        )

    ) ELSE IF "%LINK_OPTION%"=="static" (
        
        IF "%PACK_OPTION%"=="pack" (
            REM This script needs to be run from the top level mne-cpp repo folder
            REM Delete folders which we do not want to ship
            %MOCK_TEXT%rmdir %BASE_PATH%\out\\bin\resources /s /q
            %MOCK_TEXT%rmdir %BASE_PATH%\out\\bin\apps\mne_rt_server_plugins /s /q
            %MOCK_TEXT%rmdir %BASE_PATH%\out\\bin\apps\mne_scan_plugins /s /q
            %MOCK_TEXT%rmdir %BASE_PATH%\out\\bin\apps\mne_analyze_plugins /s /q
            
            REM Creating archive of everything in the bin directory
            %MOCK_TEXT%7z a %BASE_PATH%\mne-cpp-windows-static-x86_64.zip %BASE_PATH%\out\        
        )
        
    ) ELSE (
        ECHO Your link option: %LINK_OPTION% not defined.
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


EXIT_FAIL=1
EXIT_SUCCESS=0
ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BasePath="$(cleanAbsPath "$ScriptPath/..")"
OutDirName=""
LinkOption="dynamic"
PackOption=""
BuildName=""
MockDeploy="False"
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
        PackOption="pack"
    elif [ "${argv[j]}" == "help" ]; then
        PrintHelp="true"
    elif [ "${argv[j]}" == "mock" ]; then
        MockBuild="true"
    fi
    inkarg=(${${argv[j]}//=/ })
    if [ "${inkarg[0]}" == "build-name" ]; then
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
    exit(EXIT_SUCCESS)
fi

doPrintConfiguration

exit

if [[ ${LinkOption} == dynamic ]]; then

    # Call macdeployqt on all .app bundles in the bin folder
    for f in ${BasePath}/out/${BuildName}/apps/*.app; do $Qt5_DIR/bin/macdeployqt $f ; done

    # Solve for dependencies for mne_scan.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/mne_scan_plugins/. ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/MacOS/mne_scan_plugins
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/MacOS/resources
    cp -a src/applications/mne_scan/plugins/brainflowboard/brainflow/installed/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/Frameworks
    cp -a src/applications/mne_scan/plugins/lsladapter/liblsl/build/install/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/Frameworks
    cp -a ${BasePath}/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/Frameworks
    # cp -a $Qt5_DIR/plugins/renderers/. ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/PlugIns/renderers

    # Solve for dependencies for mne_analyze.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/mne_analyze_plugins/. ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/MacOS/mne_analyze_plugins
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/MacOS/resources
    cp -a ${BasePath}/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/Frameworks
    # cp -a $Qt5_DIR/plugins/renderers/. ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/PlugIns/renderers

    # Solve for dependencies for mne_rt_server.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/mne_rt_server_plugins/. ${BasePath}/out/${BuildName}/apps/mne_rt_server.app/Contents/MacOS/mne_rt_server_plugins
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_rt_server.app/Contents/MacOS/resources
    cp -a ${BasePath}/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_rt_server.app/Contents/Frameworks

    # Solve for dependencies for mne_forward_solution.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_forward_solution.app/Contents/MacOS/resources
    cp -a ${BasePath}/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_forward_solution.app/Contents/Frameworks

    # Solve for dependencies for mne_dipole_fit.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_dipole_fit.app/Contents/MacOS/resources
    cp -a ${BasePath}/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_dipole_fit.app/Contents/Frameworks

    # Solve for dependencies for mne_anonymize.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_anonymize.app/Contents/MacOS/resources
    cp -a ${BasePath}/out/${BuildName}/lib/. ${BasePath}/out/${BuildName}/apps/mne_anonymize.app/Contents/Frameworks

    if [[ ${PACK_OPTION} == pack ]]; then

        # Delete folders which we do not want to ship
        rm -r ${BasePath}/out/${BuildName}/resouces/data
        # delete these folders because they are in the macos app containers already
        rm -r ${BasePath}/out/${BuildName}/apps/mne_scan_plugins
        rm -r ${BasePath}/out/${BuildName}/apps/mne_analyze_plugins
        rm -r ${BasePath}/out/${BuildName}/apps/mne_rt_server_plugins

        # Creating archive of all macos deployed applications
        tar cfvz mne-cpp-macos-dynamic-x86_64.tar.gz ${BasePath}/out/${BuildName}/apps/.
    fi

elif [[ ${LINK_OPTION} == static ]]; then

    cd ${BASE_PATH}

    # This script needs to be run from the top level mne-cpp repo folder
    # Solve for dependencies for mne_scan.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_scan.app/Contents/MacOS/resources

    # Solve for dependencies for mne_analyze.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_analyze.app/Contents/MacOS/resources

    # Solve for dependencies for mne_rt_server.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_rt_server.app/Contents/MacOS/resources

    # Solve for dependencies for mne_forward_solution.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_forward_solution.app/Contents/MacOS/resources

    # Solve for dependencies for mne_dipole_fit.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_dipole_target_compile_definitions(mytgt PRIVATE BUILT_SHARED=$<BOOL:${BUILD_SHARED_LIBS}>) # or using `if()` for the bool conversiofit.app/Contents/MacOS/resources

    # Solve for dependencies for mne_anonymize.app bundle
    cp -a ${BasePath}/out/${BuildName}/apps/resources/. ${BasePath}/out/${BuildName}/apps/mne_anonymize.app/Contents/MacOS/resources

    if [[ ${PACK_OPTION} == pack ]]; then
        # Delete folders which we do not want to ship
        cp -r ${BasePath}/out/${BuildName}/ mne-cpp
        rm -r mne-cpp/resources/data
        rm -r mne-cpp/apps/mne_scan_plugins
        rm -r mne-cpp/apps/mne_analyze_plugins
        rm -r mne-cpp/apps/mne_rt_server_plugins

        # Creating archive of all macos deployed applications
        tar cfvz mne-cpp-macos-static-x86_64.tar.gz mne-cpp
    fi

else 
    echo "Input argument link_option is invalid."
    doPrintConfiguration
    doPrintHelp
    exit{EXIT_FAIL}
fi

    # ############## MAC SECTION ENDS ######################
    # ######################################################

elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    
    # ######################################################
    # ############## LINUX SECTION #########################

    LINK_OPTION=$1
    PACK_OPTION=$2
    SCRIPT_PATH="$(
        cd "$(dirname "$0")" >/dev/null 2>&1
        pwd -P
    )"
    BASE_PATH=${SCRIPT_PATH}/../..

    if [ -z ${LINK_OPTION} ]; then
        LINK_OPTION=dynamic
    fi

    if [[ ${LINK_OPTION} == dynamic ]]; then

        # Copy additional brainflow libs
        cp -a ${BASE_PATH}/src/applications/mne_scan/plugins/brainflowboard/brainflow/installed/out/${BuildName}/lib/. ${BASE_PATH}/out/${BuildName}/lib/

        # Copy additional LSL libs
        cp -a ${BASE_PATH}/src/applications/mne_scan/plugins/lsladapter/liblsl/build/install/out/${BuildName}/lib/. ${BASE_PATH}/out/${BuildName}/lib/

        # Install some additional packages so linuxdeployqt can find them
        sudo apt-get update
        sudo apt-get install libxkbcommon-x11-0
        sudo apt-get install libxcb-icccm4
        sudo apt-get install libxcb-image0
        sudo apt-get install libxcb-keysyms1
        sudo apt-get install libxcb-render-util0
        sudo apt-get install libbluetooth3
        sudo apt-get install libxcb-xinerama0 
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/out/${BuildName}/lib/x86_64-linux-gnu/

        cd ${BASE_PATH}

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        # linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        cd ${BASE_PATH}/mne-cpp
        ../linuxdeployqt-continuous-x86_64.AppImage ${BasePath}/out/${BuildName}/apps/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage ${BasePath}/out/${BuildName}/apps/mne_analyze -verbose2 -extra-plugins=renderers

        # Manually copy in the libxcb-xinerama library which is needed by plugins/platforms/libxcb.so
        cp /usr/out/${BuildName}/lib/x86_64-linux-gnu/libxcb-xinerama.so.0 ${BASE_PATH}/mne-cpp/out/${BuildName}/lib/

        if [[ ${PACK_OPTION} == pack ]]; then
            echo 
            echo ldd ./out/${BuildName}/apps/mne_scan
            ldd ./out/${BuildName}/apps/mne_scan

            echo 
            echo ldd ./plugins/platforms/libqxcb.so
            ldd ./plugins/platforms/libqxcb.so

            # Delete folders which we do not want to ship
            cp -r ${BasePath}/out/${BuildName}/ mne-cpp
            rm -r mne-cpp/resources/data

            # Creating archive of everything in current directory
            tar cfvz ../mne-cpp-linux-dynamic-x86_64.tar.gz mne-cpp   
        fi

    elif [[ ${LINK_OPTION} == static ]]; then

        cd ${BASE_PATH}

        sudo apt-get update
        sudo apt-get install libxkbcommon-x11-0
        sudo apt-get install libxcb-icccm4
        sudo apt-get install libxcb-image0
        sudo apt-get install libxcb-keysyms1
        sudo apt-get install libxcb-render-util0
        sudo apt-get install libbluetooth3
        sudo apt-get install libxcb-xinerama0
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/out/${BuildName}/lib/x86_64-linux-gnu/

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        # Creating a directory for linuxdeployqt to create results 
        sudo mkdir -p -m777 mne-cpp

        # linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        cd mne-cpp
        ../linuxdeployqt-continuous-x86_64.AppImage ${BasePath}/out/${BuildName}/apps/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage ${BasePath}/out/${BuildName}/apps/mne_analyze -verbose2 -extra-plugins=renderers

        echo
        echo ldd ./out/${BuildName}/apps/mne_scan
        ldd ./out/${BuildName}/apps/mne_scan

        # Delete folders which we do not want to ship
        cp -r ${BasePath}/out/ mne-cpp
        rm -r mne-cpp/resources/data
        rm -r mne-cpp/apps/mne_rt_server_plugins
        rm -r mne-cpp/apps/mne_scan_plugins
        rm -r mne-cpp/apps/mne_analyze_plugins

        if [[ ${PACK_OPTION} == pack ]]; then
            # Creating archive of everything in the bin directory
            tar cfvz ../mne-cpp-linux-static-x86_64.tar.gz mne-cpp
        fi

    else 
        echo "Input argument link_option is invalid."
        echo "Input argument link_option is set to ${LINK_OPTION}."
        echo "Use: static or dynamic"
    fi

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0

