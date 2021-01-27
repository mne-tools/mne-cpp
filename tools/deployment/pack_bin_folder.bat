:;# This is a cross-platform script
:;# Save this file with unix eol style
:;# See each operating system's "section" 

:;# Call this script with one argument which will be the type
:;# static or dynamic

:<<BATCH
    @echo off
    :; # ########## WINDOWS SECTION #########################

    SET linkOption=%1
    IF "%linkOption%"=="" (
        ECHO Variable is NOT defined
    ) ELSE (
        ECHO VAR DEFINED TO %linkOption%
    )
    SET scriptPath=%~dp0
    SET basePath=%scriptPath%..\..
    Rem Creating archive of all win deployed applications
    :; 7z a %basePath%\mne-cpp-windows-%linkOption%-x86_64.zip %basePath%\bin
    :; # ########## WINDOWS SECTION ENDS ####################
    :; # ####################################################
    exit /b
BATCH

if [ "$(uname)" == "Darwin" ]; then
    
    # ############## MAC SECTION ###########################

    # Creating archive of all macos deployed applications
    echo tar cfvz mne-cpp-macos-${1}-x86_64.tar.gz bin/.
    
    # ############## MAC SECTION ENDS ######################
    # ######################################################

elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    
    # ############## LINUX SECTION #########################

    # Creating archive of everything in current directory
    echo tar cfvz ../mne-cpp-linux-${1}-x86_64.tar.gz ./*

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0
