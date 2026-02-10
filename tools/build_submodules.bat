:;# This script downloads and builds submodules for the mne-cpp project 
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
    SET BASE_PATH=%SCRIPT_PATH%..

    FOR %%x in (%*) do (
        IF "%%~x" == "lsl" (
            cd %BASE_PATH%
            git submodule update --init src/applications/mne_scan/plugins/lsladapter/liblsl
            cd src\applications\mne_scan\plugins\lsladapter\liblsl
            mkdir build
            cd build
            cmake .. -G "Visual Studio 16 2019" -A x64
            cmake --build . --config Release --target install
        )
    )
        
    :; # ########## WINDOWS SECTION ENDS ####################
    :; # ####################################################
    exit /b
BATCH

    # ######################################################
    # ########### LINUX/MAC SECTION ########################

    SCRIPT_PATH="$(
        cd "$(dirname "$0")" >/dev/null 2>&1
        pwd -P
    )"
    BASE_PATH=${SCRIPT_PATH}/..

    argc=$#
    argv=("$@")

    for (( j=0; j<argc; j++)); do
        if [ "${argv[j]}" == "lsl" ]; then
            cd ${BASE_PATH}
            git submodule update --init src/applications/mne_scan/plugins/lsladapter/liblsl
            cd src/applications/mne_scan/plugins/lsladapter/liblsl
            mkdir build
            cd build
            cmake ..
            cmake --build .
        fi
    done

    # ########### LINUX/MAC SECTION ENDS ###################
    # ######################################################

exit 0
