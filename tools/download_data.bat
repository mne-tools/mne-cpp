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

    setlocal EnableDelayedExpansion

    SET ScriptPath=%~dp0
    SET BasePath=%ScriptPath%..

    SET "EXIT_FAIL=1"
    SET "EXIT_SUCCESS=0"

    SET ResourcesPath=%BasePath%\resources
    SET "DownloadTestData=False"
    SET "DownloadSampleData=False"

    :loop
    IF NOT "%1"=="" (
      IF "%1"=="all" (
        SET "DownloadTestData=True"
        SET "DownloadSampleData=True"
      )
      IF "%1"=="test-data" (
        SET "DownloadTestData=True"
      )
      IF "%1"=="sample-data" (
        SET "DownloadSampleData=True"
      )
      IF "%1"=="help" (
          call:showHelp
          goto:endOfScript
      )
      SHIFT
      GOTO :loop
    )

    call:doPrintConfiguration

    IF "%DownloadTestData%"=="True" (
        git clone https://github.com/mne-tools/mne-cpp-test-data.git %ResourcesPath%\data\mne-cpp-test-data
    )
    IF "%DownloadSampleData%"=="True" (
        currentFolder=%cd%
        cd %ResourcesPath%\data
        ECHO Downloading sample data ...
        powershell.exe -Command "(new-object System.Net.WebClient).DownloadFile('https://files.osf.io/v1/resources/rxvq7/providers/osfstorage/59c0e26f9ad5a1025c4ab159', 'MNE-sample-data.tar.gz')"
        ECHO Uncompressing data ...
        tar -xf MNE-sample-data.tar.gz
        symlink /D MNE-sample-data\subjects\sample\bem\inner_skull.surf MNE-sample-data\subjects\sample\bem\flash\inner_skull.surf
        symlink /D MNE-sample-data\subjects\sample\bem\outer_skull.surf MNE-sample-data\subjects\sample\bem\flash\outer_skull.surf
        symlink /D MNE-sample-data\subjects\sample\bem\outer_skin.surf MNE-sample-data\subjects\sample\bem\flash\outer_skin.surf
        rm -f MNE-sample-data.tar.gz
        cd %currentFolder%
    )

:endOfScript

exit /B 

:doPrintConfiguration
  ECHO.
  ECHO ====================================================================
  echo ======================== MNE-CPP DOWNLOAD DATA =====================
  ECHO.
  ECHO ScriptPath    = %ScriptPath%
  ECHO BasePath      = %BasePath%
  ECHO ResourcesPath = %ResourcesPath%
  ECHO.
  ECHO DownloadTestData   = %DownloadTestData%
  ECHO DownloadSampleData = %DownloadSampleData%
  ECHO.
  ECHO ====================================================================
  ECHO ====================================================================
  ECHO.
exit /B 0

:showHelp
  ECHO. 
  ECHO MNE-CPP download data script help.
  ECHO. 
  ECHO Usage: ./download_data.bat [Options]
  ECHO.
  ECHO All builds will be parallel.
  ECHO All options can be used in undefined order.
  ECHO.
  ECHO [help]  - Print this help.
  ECHO [all]   - Download all datasets.
  ECHO [test-data]   - Download mne-cpp-test-data.
  ECHO [sample-data] - Download Sample dataset.
  ECHO.
exit /B 0


    :; # ########## WINDOWS SECTION ENDS ####################
    :; #####################################################
    exit /b
BATCH

#if [ "$(uname)" == "Darwin" ]; then
    
    # ######################################################
    # ############## LINUX MAC SECTION ###########################

function cleanAbsPath()
{
    local  cleanAbsPathStr="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$cleanAbsPathStr"
}

argc=$#
argv=("$@")
EXIT_SUCCESS=0
EXIT_FAIL=1
ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BasePath="$(cleanAbsPath "$ScriptPath/..")"
ResourcesPath="$(cleanAbsPath "$BasePath/resources")"
DownloadTestData="false" 
DownloadSampleData="false" 

doPrintConfiguration() {
  echo " "
  echo ========================================================================
  echo ======================== MNE-CPP DOWNLOAD DATA ==========================
  echo " "
  echo " ScriptPath = $ScriptPath"
  echo " BasePath   = $BasePath"
  echo " ResourcesPath = $ResourcesPath"
  echo " "
  echo " DownloadTestData = $DownloadTestData"
  echo " DownloadSampleData  = $DownloadSampleData"
  echo " "
  echo ========================================================================
  echo ========================================================================
  echo " "
}

doPrintHelp() {
  echo " "
  echo "Download sample or test datasets into the resources/data folder. "
  echo "Usage: ./deploy.bat [Options]"
  echo " "
  echo "All options can be used in undefined order."
  echo " "
  echo "[help] - Print this help."
  echo "[all]  - Download all datasets."
  echo "[test-data]   - Download mne-cpp-test-data set."
  echo "[sample-data] - Download MNE Sample Data set."
  echo " "
}

#parse input args
if [ $argc -eq 0 ]; then
  PrintHelp="true"
fi

for (( j=0; j<argc; j++ )); do
    if [ "${argv[j]}" == "all" ]; then
        DownloadTestData="true"
        DownloadSampleData="true"
    elif [ "${argv[j]}" == "test-data" ]; then
        DownloadTestData="true"
    elif [ "${argv[j]}" == "sample-data" ]; then
        DownloadSampleData="true"
    elif [ "${argv[j]}" == "help" ]; then
        PrintHelp="true"
    fi
done

#execution starts here
if [ "${PrintHelp}" == "true" ]; then
    doPrintHelp
    exit ${EXIT_SUCCESS} 
fi

doPrintConfiguration

if [[ ${DownloadTestData} == "true" ]]; then
  git clone https://github.com/mne-tools/mne-cpp-test-data.git ${ResourcesPath}/data/mne-cpp-test-data
fi
if [[ ${DownloadSampleData} == "true" ]]; then
  curl -O https://files.osf.io/v1/resources/rxvq7/providers/osfstorage/59c0e26f9ad5a1025c4ab159\?action\=download\&direct\&version\=6 
  mv 59c0e26f9ad5a1025c4ab159 ${ResourcesPath}/data/MNE-sample-data.tar.gz
  currentFolder=`pwd`
  cd ${ResourcesPath}/data/
  tar -xvf MNE-sample-data.tar.gz 
  cd ${currentFolder}
fi

    # ############## LINUX MAC SECTION ENDS ######################
    # ######################################################

#elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    
#fi

exit 0

