:;# This is a cross-platform script
:;# Save this file with unix eol style
:;# See each operating system's "section" 

:;# Call this script with one argument which will be the type
:;# static or dynamic

:<<BATCH
    :;@echo off
    :; # ########## WINDOWS SECTION #########################

    SET SCRIPT_PATH=%~dp0
    SET BASE_PATH=%SCRIPT_PATH%..\..
    SET LINK_OPTION=%1
    SET PACK_OPTION=%2
    
    :;IF "%LINK_OPTION%"=="" (
    :;    SET LINK_OPTION=dynamic
    :;)

    IF "%LINK_OPTION%"=="dynamic" (
        
        Rem Solve for dependencies only mne_scan.exe and mnecppDisp3D.dll since it links all needed qt and mne-cpp libs
        windeployqt %BASE_PATH%\bin\mne_scan.exe
        windeployqt %BASE_PATH%\bin\mnecppDisp3D.dll
        Rem Copy LSL and Brainflowlibraries manually
        xcopy %BASE_PATH%\applications\mne_scan\plugins\brainflowboard\brainflow\installed\lib\* %BASE_PATH%\bin\ /s /i
        xcopy %BASE_PATH%\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll %BASE_PATH%\bin\ /i
        
        IF "%PACK_OPTION%"=="pack" (
            Rem Delete folders which we do not want to ship
            Remove-Item '%BASE_PATH%\bin\mne-cpp-test-data' -Recurse
            Rem Creating archive of all win deployed applications
            7z a %BAS_PATH%\mne-cpp-windows-dynamic-x86_64.zip %BASE_PATH%/bin
        )

    ) ELSE IF "%LINK_OPTION%"=="static" (
        
        IF "%PACK_OPTION%"=="pack" (
            Rem This script needs to be run from the top level mne-cpp repo folder
            Rem Delete folders which we do not want to ship
            Remove-Item '%BASE_PATH%\bin\mne_rt_server_plugins' -Recurse
            Remove-Item '%BASE_PATH%\bin\mne-cpp-test-data' -Recurse
            Remove-Item '%BASE_PATH%\bin\mne_scan_plugins' -Recurse
            Remove-Item '%BASE_PATH%\bin\mne_analyze_plugins' -Recurse
            
            Rem Creating archive of everything in the bin directory
            7z a mne-cpp-windows-static-x86_64.zip ./bin        
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

    if [[ ${LINK_OPTION} == dynamic ]]; then

        cd ${BASE_PATH}

        # Call macdeployqt on all .app bundles in the bin folder
        for f in ./bin/*.app; do $Qt5_DIR/bin/macdeployqt $f ; done

        # Solve for dependencies for mne_scan.app bundle
        cp -a bin/mne_scan_plugins/. bin/mne_scan.app/Contents/MacOS/mne_scan_plugins
        cp -a bin/resources/. bin/mne_scan.app/Contents/MacOS/resources
        cp -a applications/mne_scan/plugins/brainflowboard/brainflow/installed/lib/. bin/mne_scan.app/Contents/Frameworks
        cp -a applications/mne_scan/plugins/lsladapter/liblsl/build/install/lib/. bin/mne_scan.app/Contents/Frameworks
        cp -a lib/. bin/mne_scan.app/Contents/Frameworks
        # cp -a $Qt5_DIR/plugins/renderers/. bin/mne_scan.app/Contents/PlugIns/renderers

        # Solve for dependencies for mne_analyze.app bundle
        cp -a bin/mne_analyze_plugins/. bin/mne_analyze.app/Contents/MacOS/mne_analyze_plugins
        cp -a bin/resources/. bin/mne_analyze.app/Contents/MacOS/resources
        cp -a lib/. bin/mne_analyze.app/Contents/Frameworks
        # cp -a $Qt5_DIR/plugins/renderers/. bin/mne_analyze.app/Contents/PlugIns/renderers

        # Solve for dependencies for mne_rt_server.app bundle
        cp -a bin/mne_rt_server_plugins/. bin/mne_rt_server.app/Contents/MacOS/mne_rt_server_plugins
        cp -a bin/resources/. bin/mne_rt_server.app/Contents/MacOS/resources
        cp -a lib/. bin/mne_rt_server.app/Contents/Frameworks

        # Solve for dependencies for mne_forward_solution.app bundle
        cp -a bin/resources/. bin/mne_forward_solution.app/Contents/MacOS/resources
        cp -a lib/. bin/mne_forward_solution.app/Contents/Frameworks

        # Solve for dependencies for mne_dipole_fit.app bundle
        cp -a bin/resources/. bin/mne_dipole_fit.app/Contents/MacOS/resources
        cp -a lib/. bin/mne_dipole_fit.app/Contents/Frameworks

        # Solve for dependencies for mne_anonymize.app bundle
        cp -a bin/resources/. bin/mne_anonymize.app/Contents/MacOS/resources
        cp -a lib/. bin/mne_anonymize.app/Contents/Frameworks


        if [[ ${PACK_OPTION} == pack ]]; then

            # Delete folders which we do not want to ship
            rm -r bin/mne-cpp-test-data
            rm -r bin/mne_scan_plugins
            rm -r bin/mne_analyze_plugins
            rm -r bin/mne_rt_server_plugins
            rm -r bin/resources

            # Creating archive of all macos deployed applications
            tar cfvz mne-cpp-macos-dynamic-x86_64.tar.gz bin/.
        fi


    elif [[ ${LINK_OPTION} == static ]]; then

        cd ${BASE_PATH}

        # This script needs to be run from the top level mne-cpp repo folder
        # Solve for dependencies for mne_scan.app bundle
        cp -a bin/resources/. bin/mne_scan.app/Contents/MacOS/resources

        # Solve for dependencies for mne_analyze.app bundle
        cp -a bin/resources/. bin/mne_analyze.app/Contents/MacOS/resources

        # Solve for dependencies for mne_rt_server.app bundle
        cp -a bin/resources/. bin/mne_rt_server.app/Contents/MacOS/resources

        # Solve for dependencies for mne_forward_solution.app bundle
        cp -a bin/resources/. bin/mne_forward_solution.app/Contents/MacOS/resources

        # Solve for dependencies for mne_dipole_fit.app bundle
        cp -a bin/resources/. bin/mne_dipole_fit.app/Contents/MacOS/resources

        # Solve for dependencies for mne_anonymize.app bundle
        cp -a bin/resources/. bin/mne_anonymize.app/Contents/MacOS/resources

        if [[ ${PACK_OPTION} == pack ]]; then
            # Delete folders which we do not want to ship
            rm -r bin/mne-cpp-test-data
            rm -r bin/mne_scan_plugins
            rm -r bin/mne_analyze_plugins
            rm -r bin/mne_rt_server_plugins
            rm -r bin/resources

            # Creating archive of all macos deployed applications
            tar cfvz mne-cpp-macos-static-x86_64.tar.gz bin/.
        fi

    elif [ -z ${LINK_OPTION} ]; then
        echo "Input argument link_option is not set."
        echo "Use: static or dynamic"
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

    if [[ ${LINK_OPTION} == dynamic ]]; then

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
        ../linuxdeployqt-continuous-x86_64.AppImage bin/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage bin/mne_analyze -verbose2 -extra-plugins=renderers

        # Manually copy in the libxcb-xinerama library which is needed by plugins/platforms/libxcb.so
        cp /usr/lib/x86_64-linux-gnu/libxcb-xinerama.so.0 ${BASE_PATH}/mne-cpp/lib/

        if [[ ${PACK_OPTION} == pack ]]; then
            echo 
            echo ldd ./bin/mne_scan
            ldd ./bin/mne_scan

            echo 
            echo ldd ./plugins/platforms/libqxcb.so
            ldd ./plugins/platforms/libqxcb.so

            # Delete folders which we do not want to ship
            rm -r bin/mne-cpp-test-data

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
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu/

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        # Creating a directory for linuxdeployqt to create results 
        sudo mkdir -p -m777 mne-cpp

        # Copying built data to folder for easy packaging   
        cp -r ./bin ./lib mne-cpp/

        # linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        cd mne-cpp
        ../linuxdeployqt-continuous-x86_64.AppImage bin/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage bin/mne_analyze -verbose2 -extra-plugins=renderers

        echo
        echo ldd ./bin/mne_scan
        ldd ./bin/mne_scan

        # Delete folders which we do not want to ship
        rm -r bin/mne_rt_server_plugins
        rm -r bin/mne-cpp-test-data
        rm -r bin/mne_scan_plugins
        rm -r bin/mne_analyze_plugins

        if [[ ${PACK_OPTION} == pack ]]; then
            # Creating archive of everything in the bin directory
            tar cfvz ../mne-cpp-linux-static-x86_64.tar.gz bin/. lib/.
        fi

        rm -fr mne-cpp

    elif [ -z ${LINK_OPTION} ]; then
        echo "Input argument link_option is not set."
        echo "Use: static or dynamic"
    else 
        echo "Input argument link_option is invalid."
        echo "Input argument link_option is set to ${LINK_OPTION}."
        echo "Use: static or dynamic"
    fi

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0
