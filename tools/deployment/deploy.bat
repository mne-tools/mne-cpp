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
    :; # ########## WINDOWS SECTION #########################

    SET SCRIPT_PATH=%~dp0
    SET BASE_PATH=%SCRIPT_PATH%..\..
    SET LINK_OPTION=%1
    SET PACK_OPTION=%2
    
    SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\"

    IF "%LINK_OPTION%"=="" (
        SET LINK_OPTION=dynamic
    )

    IF "%LINK_OPTION%"=="dynamic" (
        
        Rem Solve for dependencies only mne_scan.exe and mnecppDisp3D.dll since it links all needed qt and mne-cpp libs
        windeployqt %BASE_PATH%\out\Release\apps\mne_scan.exe
        windeployqt %BASE_PATH%\out\Release\apps\mnecppDisp3D.dll
        Rem Copy LSL and Brainflowlibraries manually
        xcopy %BASE_PATH%\src\applications\mne_scan\plugins\brainflowboard\brainflow\installed\lib\* %BASE_PATH%\out\Release\apps\ /s /i
        xcopy %BASE_PATH%\src\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll %BASE_PATH%\out\Release\apps\ /i
        
        IF "%PACK_OPTION%"=="pack" (
            Rem Delete folders which we do not want to ship
            rmdir %BASE_PATH%\out\Release\resources\data /s /q 
            Rem Creating archive of all win deployed applications
            7z a %BASE_PATH%\mne-cpp-windows-dynamic-x86_64.zip %BASE_PATH%\out\Release
        )

    ) ELSE IF "%LINK_OPTION%"=="static" (
        
        IF "%PACK_OPTION%"=="pack" (
            Rem This script needs to be run from the top level mne-cpp repo folder
            Rem Delete folders which we do not want to ship
            rmdir %BASE_PATH%\out\Release\apps\mne_rt_server_plugins /s /q
            rmdir %BASE_PATH%\out\Release\resources\data /s /q
            rmdir %BASE_PATH%\out\Release\apps\mne_scan_plugins /s /q
            rmdir %BASE_PATH%\out\Release\apps\mne_analyze_plugins /s /q
            
            Rem Creating archive of everything in the bin directory
            7z a %BASE_PATH%\mne-cpp-windows-static-x86_64.zip %BASE_PATH%\out\Release        
        )
        
    ) ELSE (
        ECHO Your link option: %LINK_OPTION%
        ECHO Linkage option not defined. 
        ECHO Use: static or dynamic.
    )
        
    :; # ########## WINDOWS SECTION ENDS ####################
    :; # ####################################################
    exit /b
BATCH

if [ "$(uname)" == "Darwin" ]; then
    
    # ######################################################
    # ############## MAC SECTION ###########################

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

        cd ${BASE_PATH}

        # Call macdeployqt on all .app bundles in the bin folder
        for f in ./out/Release/apps/*.app; do $Qt5_DIR/bin/macdeployqt $f ; done

        # Solve for dependencies for mne_scan.app bundle
        cp -a out/Release/apps/mne_scan_plugins/. out/Release/apps/mne_scan.app/Contents/MacOS/mne_scan_plugins
        cp -a out/Release/apps/resources/. out/Release/apps/mne_scan.app/Contents/MacOS/resources
        cp -a src/applications/mne_scan/plugins/brainflowboard/brainflow/installed/out/Release/lib/. out/Release/apps/mne_scan.app/Contents/Frameworks
        cp -a src/applications/mne_scan/plugins/lsladapter/liblsl/build/install/out/Release/lib/. out/Release/apps/mne_scan.app/Contents/Frameworks
        cp -a out/Release/lib/. out/Release/apps/mne_scan.app/Contents/Frameworks
        # cp -a $Qt5_DIR/plugins/renderers/. out/Release/apps/mne_scan.app/Contents/PlugIns/renderers

        # Solve for dependencies for mne_analyze.app bundle
        cp -a out/Release/apps/mne_analyze_plugins/. out/Release/apps/mne_analyze.app/Contents/MacOS/mne_analyze_plugins
        cp -a out/Release/apps/resources/. out/Release/apps/mne_analyze.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_analyze.app/Contents/Frameworks
        # cp -a $Qt5_DIR/plugins/renderers/. out/Release/apps/mne_analyze.app/Contents/PlugIns/renderers

        # Solve for dependencies for mne_rt_server.app bundle
        cp -a out/Release/apps/mne_rt_server_plugins/. out/Release/apps/mne_rt_server.app/Contents/MacOS/mne_rt_server_plugins
        cp -a out/Release/apps/resources/. out/Release/apps/mne_rt_server.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_rt_server.app/Contents/Frameworks

        # Solve for dependencies for mne_forward_solution.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_forward_solution.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_forward_solution.app/Contents/Frameworks

        # Solve for dependencies for mne_dipole_fit.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_dipole_fit.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_dipole_fit.app/Contents/Frameworks

        # Solve for dependencies for mne_anonymize.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_anonymize.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_anonymize.app/Contents/Frameworks

        if [[ ${PACK_OPTION} == pack ]]; then

            # Delete folders which we do not want to ship
            rm -r out/Release/resouces/data
            rm -r out/Release/apps/mne_scan_plugins
            rm -r out/Release/apps/mne_analyze_plugins
            rm -r out/Release/apps/mne_rt_server_plugins

            # Creating archive of all macos deployed applications
            tar cfvz mne-cpp-macos-dynamic-x86_64.tar.gz out/Release/apps/.
        fi

    elif [[ ${LINK_OPTION} == static ]]; then

        cd ${BASE_PATH}

        # This script needs to be run from the top level mne-cpp repo folder
        # Solve for dependencies for mne_scan.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_scan.app/Contents/MacOS/resources

        # Solve for dependencies for mne_analyze.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_analyze.app/Contents/MacOS/resources

        # Solve for dependencies for mne_rt_server.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_rt_server.app/Contents/MacOS/resources

        # Solve for dependencies for mne_forward_solution.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_forward_solution.app/Contents/MacOS/resources

        # Solve for dependencies for mne_dipole_fit.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_dipole_fit.app/Contents/MacOS/resources

        # Solve for dependencies for mne_anonymize.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_anonymize.app/Contents/MacOS/resources

        if [[ ${PACK_OPTION} == pack ]]; then
            # Delete folders which we do not want to ship
            rm -r out/Release/resources/data
            rm -r out/Release/apps/mne_scan_plugins
            rm -r out/Release/apps/mne_analyze_plugins
            rm -r out/Release/apps/mne_rt_server_plugins

            # Creating archive of all macos deployed applications
            tar cfvz mne-cpp-macos-static-x86_64.tar.gz out/Release/apps/.
        fi

    else 
        echo "Input argument link_option is invalid."
        echo "Input argument link_option is set to ${LINK_OPTION}."
        echo "Use: static or dynamic"
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
        cp -a ${BASE_PATH}/src/applications/mne_scan/plugins/brainflowboard/brainflow/installed/out/Release/lib/. ${BASE_PATH}/out/Release/lib/

        # Copy additional LSL libs
        cp -a ${BASE_PATH}/src/applications/mne_scan/plugins/lsladapter/liblsl/build/install/out/Release/lib/. ${BASE_PATH}/out/Release/lib/

        # Install some additional packages so linuxdeployqt can find them
        sudo apt-get update
        sudo apt-get install libxkbcommon-x11-0
        sudo apt-get install libxcb-icccm4
        sudo apt-get install libxcb-image0
        sudo apt-get install libxcb-keysyms1
        sudo apt-get install libxcb-render-util0
        sudo apt-get install libbluetooth3
        sudo apt-get install libxcb-xinerama0 
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/out/Release/lib/x86_64-linux-gnu/

        cd ${BASE_PATH}

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        # Creating a directory for linuxdeployqt to create results 
        sudo mkdir -p -m777 mne-cpp

        # Copying built data to folder for easy packaging   
        cp -r ${BASE_PATH}/bin ${BASE_PATH}/lib ${BASE_PATH}/mne-cpp/

        # linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        cd ${BASE_PATH}/mne-cpp
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_analyze -verbose2 -extra-plugins=renderers

        # Manually copy in the libxcb-xinerama library which is needed by plugins/platforms/libxcb.so
        cp /usr/out/Release/lib/x86_64-linux-gnu/libxcb-xinerama.so.0 ${BASE_PATH}/mne-cpp/out/Release/lib/

        if [[ ${PACK_OPTION} == pack ]]; then
            echo 
            echo ldd ./out/Release/apps/mne_scan
            ldd ./out/Release/apps/mne_scan

            echo 
            echo ldd ./plugins/platforms/libqxcb.so
            ldd ./plugins/platforms/libqxcb.so

            # Delete folders which we do not want to ship
            rm -r out/Release/resources/data

            # Creating archive of everything in current directory
            tar cfvz ../mne-cpp-linux-dynamic-x86_64.tar.gz ./*    
        fi
        rm -fr mne-cpp

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
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/out/Release/lib/x86_64-linux-gnu/

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        # Creating a directory for linuxdeployqt to create results 
        sudo mkdir -p -m777 mne-cpp

        # Copying built data to folder for easy packaging   
        cp -r ./bin ./lib mne-cpp/

        # linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        cd mne-cpp
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_analyze -verbose2 -extra-plugins=renderers

        echo
        echo ldd ./out/Release/apps/mne_scan
        ldd ./out/Release/apps/mne_scan

        # Delete folders which we do not want to ship
        rm -r out/Release/apps/mne_rt_server_plugins
        rm -r out/Release/resources/data
        rm -r out/Release/apps/mne_scan_plugins
        rm -r out/Release/apps/mne_analyze_plugins

        if [[ ${PACK_OPTION} == pack ]]; then
            # Creating archive of everything in the bin directory
            tar cfvz ../mne-cpp-linux-static-x86_64.tar.gz out/Release/apps/. out/Release/lib/.
        fi

        rm -fr mne-cpp

    else 
        echo "Input argument link_option is invalid."
        echo "Input argument link_option is set to ${LINK_OPTION}."
        echo "Use: static or dynamic"
    fi

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0

