:;# This is a cross-platform script
:;# Save this file with unix eol style
:;# See each operating system's "section" 

:;# Call this script with one argument which will be the type
:;# static or dynamic

:<<BATCH
    @echo off
    :; # ########## WINDOWS SECTION #########################

    SET SCRIPT_PATH=%~dp0
    SET BASE_PATH=%SCRIPT_PATH%..\..
    SET LINK_OPTION=%1
    IF "%LINK_OPTION%"=="" (
        ECHO Linkage option not defined. 
        ECHO Use: static or dynamic.
    ) ELSE (
        IF "%LINK_OPTION%"=="dynamic" (
            Rem Solve for dependencies only mne_scan.exe and mnecppDisp3D.dll since it links all needed qt and mne-cpp libs
            windeployqt %BASE_PATH%\bin\mne_scan.exe
            windeployqt %BASE_PATH%\bin\mnecppDisp3D.dll

            Rem Copy LSL and Brainflowlibraries manually
            xcopy %BASE_PATH%\applications\mne_scan\plugins\brainflowboard\brainflow\installed\lib\* %BASE_PATH%\bin\ /s /i
            xcopy %BASE_PATH%\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll %BASE_PATH%\bin\ /i
        )
    )
    :; # ########## WINDOWS SECTION ENDS ####################
    :; # ####################################################
    exit /b
BATCH

if [ "$(uname)" == "Darwin" ]; then
    
    # ############## MAC SECTION ###########################

    LINK_OPTION=$1
    SCRIPT_PATH="$(
        cd "$(dirname "$0")" >/dev/null 2>&1
        pwd -P
    )"
    BASE_PATH=${SCRIPT_PATH}/../..

    if [ -z ${LINK_OPTION} ]; then
        echo "Variable ${LINK_OPTION} is not set."
        echo "Use: static or dynamic"
    else
        if [ ${LINK_OPTION} == "dynamic" ]; then
            # Call macdeployqt on all .app bundles in the bin folder
            for f in ./bin/*.app; do $Qt5_DIR/bin/macdeployqt $f ; done

            # Solve for dependencies for mne_scan.app bundle
            cp -a ${SCRIPT_PATH}/bin/mne_scan_plugins/. ${SCRIPT_PATH}/bin/mne_scan.app/Contents/MacOS/mne_scan_plugins
            cp -a ${SCRIPT_PATH}/applications/mne_scan/plugins/brainflowboard/brainflow/installed/lib/. ${SCRIPT_PATH}/bin/mne_scan.app/Contents/Frameworks
            cp -a ${SCRIPT_PATH}/applications/mne_scan/plugins/lsladapter/liblsl/build/install/lib/. ${SCRIPT_PATH}/bin/mne_scan.app/Contents/Frameworks
            cp -a ${SCRIPT_PATH}/lib/. ${SCRIPT_PATH}/bin/mne_scan.app/Contents/Frameworks
            # cp -a $Qt5_DIR/plugins/renderers/. bin/mne_scan.app/Contents/PlugIns/renderers

            # Solve for dependencies for mne_analyze.app bundle
            cp -a ${SCRIPT_PATH}/bin/mne_analyze_plugins/. ${SCRIPT_PATH}/bin/mne_analyze.app/Contents/MacOS/mne_analyze_plugins
            cp -a ${SCRIPT_PATH}/lib/. bin/mne_analyze.app/Contents/Frameworks
            # cp -a $Qt5_DIR/plugins/renderers/. ${SCRIPT_PATH}/bin/mne_analyze.app/Contents/PlugIns/renderers

            # Solve for dependencies for mne_rt_server.app bundle
            cp -a ${SCRIPT_PATH}/bin/mne_rt_server_plugins/. ${SCRIPT_PATH}/bin/mne_rt_server.app/Contents/MacOS/mne_rt_server_plugins
            cp -a ${SCRIPT_PATH}/lib/. ${SCRIPT_PATH}/bin/mne_rt_server.app/Contents/Frameworks

            # Solve for dependencies for mne_forward_solution.app bundle
            cp -a ${SCRIPT_PATH}/lib/. ${SCRIPT_PATH}/bin/mne_forward_solution.app/Contents/Frameworks

            # Solve for dependencies for mne_dipole_fit.app bundle
            cp -a ${SCRIPT_PATH}/lib/. ${SCRIPT_PATH}/bin/mne_dipole_fit.app/Contents/Frameworks

            # Solve for dependencies for mne_anonymize.app bundle
            cp -a ${SCRIPT_PATH}/lib/. ${SCRIPT_PATH}/bin/mne_anonymize.app/Contents/Frameworks
        fi

        # These commands run both in dynamic and static deployments

        # Solve for dependencies for mne_scan.app bundle
        cp -a ${SCRIPT_PATH}/bin/resources/. ${SCRIPT_PATH}/bin/mne_scan.app/Contents/MacOS/resources

        # Solve for dependencies for mne_analyze.app bundle
        cp -a ${SCRIPT_PATH}/bin/resources/. ${SCRIPT_PATH}/bin/mne_analyze.app/Contents/MacOS/resources

        # Solve for dependencies for mne_rt_server.app bundle
        cp -a ${SCRIPT_PATH}/bin/resources/. ${SCRIPT_PATH}/bin/mne_rt_server.app/Contents/MacOS/resources

        # Solve for dependencies for mne_forward_solution.app bundle
        cp -a ${SCRIPT_PATH}/bin/resources/. ${SCRIPT_PATH}/bin/mne_forward_solution.app/Contents/MacOS/resources

        # Solve for dependencies for mne_dipole_fit.app bundle
        cp -a ${SCRIPT_PATH}/bin/resources/. ${SCRIPT_PATH}/bin/mne_dipole_fit.app/Contents/MacOS/resources

        # Solve for dependencies for mne_anonymize.app bundle
        cp -a ${SCRIPT_PATH}/bin/resources/. ${SCRIPT_PATH}/bin/mne_anonymize.app/Contents/MacOS/resources

    fi

    # ############## MAC SECTION ENDS ######################
    # ######################################################

elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    
    # ############## LINUX SECTION #########################


    LINK_OPTION=$1
    SCRIPT_PATH="$(
        cd "$(dirname "$0")" >/dev/null 2>&1
        pwd -P
    )"
    BASE_PATH=${SCRIPT_PATH}/../..
    if [ -z ${LINK_OPTION} ]; then
        echo "Variable ${LINK_OPTION} is not set."
        echo "Use: static or dynamic"
    else
        # Copy additional brainflow libs
        cp -a ${BASE_PATH}/applications/mne_scan/plugins/brainflowboard/brainflow/installed/lib/. ${BASE_PATH}/lib/

        # Copy additional LSL libs
        cp -a ${BASE_PATH}/applications/mne_scan/plugins/lsladapter/liblsl/build/install/lib/. ${BASE_PATH}/lib/

        # Install some additional packages so linuxdeployqt can find them
        sudo apt-get update
        sudo apt-get install libxkbcommon-x11-0
        sudo apt-get install libxcb-icccm4
        sudo apt-get install libxcb-image0
        sudo apt-get install libxcb-keysyms1
        sudo apt-get install libxcb-render-util0
        sudo apt-get install libbluetooth3
        sudo apt-get install libxcb-xinerama0 
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu/

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        ## Creating a directory for linuxdeployqt to create results         
        ## If there is already such directory, delete it and start over.
        #if [ -d "mne-cpp_deploy" ]; then
        #    rm -r mne-cpp_deploy
        #fi
        #sudo mkdir -p -m777 mne-cpp_deploy

        ## Copying built data to folder for easy packaging   
        #cp -r ${BASE_PATH}/bin ${BASE_PATH}/lib mne-cpp_deploy/
        #cd mne-cpp_deploy

        ## linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        linuxdeployqt-continuous-x86_64.AppImage ${BASE_PATH}/bin/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage ${BASE_PATH}/bin/mne_analyze -verbose2 -extra-plugins=renderers

        # Manually copy in the libxcb-xinerama library which is needed by plugins/platforms/libxcb.so
        cp /usr/lib/x86_64-linux-gnu/libxcb-xinerama.so.0 ${BASE_PATH}/lib/

        echo 
        echo ldd ./bin/mne_scan
        ldd ${BASE_PATH}/bin/mne_scan

        echo 
        echo ldd ./plugins/platforms/libqxcb.so
        ldd ${BASE_PATH}/plugins/platforms/libqxcb.so
    fi

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0
