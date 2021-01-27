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
        Rem Delete folders which we do not want to ship
        Remove-Item '%BASE_PATH%\bin\mne-cpp-test-data' -Recurse
        Remove-Item '%BASE_PATH%\bin\mne_scan_plugins' -Recurse
        Remove-Item '%BASE_PATH%\bin\mne_analyze_plugins' -Recurse
        Remove-Item '%BASE_PATH%\bin\mne_rt_server_plugins' -Recurse
        Remove-Item '%BASE_PATH%\bin\resources' -Recurse

        Rem Creating archive of all win deployed applications
        7z a %BASE_PATH%\mne-cpp-windows-%LINK_OPTION%-x86_64.zip %BASE_PATH%\bin
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
        tar cfvz ${BASE_PATH}/mne-cpp-linux-${LINK_OPTION}-x86_64.tar.gz ${BASE_PATH}/bin/. ${BASE_PATH}/lib/.
    fi

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0
