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
    
    :;IF "%LINK_OPTION%"=="" (
    :;    SET LINK_OPTION=dynamic
    :;) 

    IF "%LINK_OPTION%"=="dynamic" || "%LINK_OPTION%"=="static" (
        
        7z a %BASE_PATH%\mne-cpp-windows-%LINK_OPTION%-x86_64.zip %BASE_PATH%\bin

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
        # Delete folders which we do not want to ship
        rm -r ${BASE_PATH}/bin/mne-cpp-test-data
        rm -r ${BASE_PATH}/bin/mne_scan_plugins
        rm -r ${BASE_PATH}/bin/mne_analyze_plugins
        rm -r ${BASE_PATH}/bin/mne_rt_server_plugins
        rm -r ${BASE_PATH}/bin/resources

        # Creating archive of everything in current directory
        tar cfvz ${BASE_PATH}/mne-cpp-macos-${LINK_OPTION}-x86_64.tar.gz ${BASE_PATH}/bin/.
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

    elif [[ ${LINK_OPTION} == dynamic ]]; then

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

        # Manually copy in the libxcb-xinerama library which is needed by plugins/platforms/libxcb.so
        cp /usr/lib/x86_64-linux-gnu/libxcb-xinerama.so.0 ./lib/

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

    elif [[ ${LINK_OPTION} == static ]]; then

        cd %{BASE_PATH}/mne-cpp

        # Delete folders which we do not want to ship
        rm -r bin/mne_rt_server_plugins
        rm -r bin/mne-cpp-test-data
        rm -r bin/mne_scan_plugins
        rm -r bin/mne_analyze_plugins

        # Creating archive of everything in the bin directory
        tar cfvz ../mne-cpp-linux-static-x86_64.tar.gz bin/. lib/.

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
