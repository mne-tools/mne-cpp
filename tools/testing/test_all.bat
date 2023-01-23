
:;# This script performs generates and copies the necesary library dependencies for running qt-projects both for 
:;# dynamic and for staic builds. 
:;#
:;# This file is part of the MNE-CPP project. For more information visit: https://mne-cpp.github.io/
:;#
:;# This script is based ßon an open-source cross-platform script template.
:;# For more information you can visit: https://github.com/juangpc/multiplatform_bash_cmd
:;# 

:<<BATCH
  :; # ########## WINDOWS SECTION #########################
  @echo off
  
@REM Setup default values
setlocal
set scriptPath=%~dp0
set basePath=%scriptPath%..\..

set printOutput=False
set stopOnFirstFail=False
set runCodeCoverage=False

@REM parse input argument
set verboseModeInput=%1
set runCodeCoverageInput=%2

if "%verboseModeInput%"=="verbose" (
  set printOutput=True
) else (
  if "%verboseModeInput%"=="help" (
    call:doPrintHelp
    exit /B 0
  )
)

if "%runCodeCoverageInput%"=="withCoverage" (
  set runCodeCoverage=True
)

@REM start execution

call:doPrintConfiguration

set /A "compoundOutput=0"

cd %basePath%\bin

for /f %%f in ('dir test_*.exe /s /b ^| findstr /v "d.exe"') do (
  if "%printOutput%"=="True" (
    %%f && call:success %%~nxf || call:fail %%~nxf
  ) else (
    %%f > null 2>&1 && call:success %%~nxf || call:fail %%~nxf
  )
)

cd %scriptPath%

exit /B %compoundOutput%

:; # this part should never get parsed other than the actual
:; # functions defined herein.

:doPrintConfiguration
  echo.
  echo =========================================
  echo verbose = %printOutput%
  echo exitOnFail = %stopOnFirstFail%
  echo runCodeCoverage = %runCodeCoverage%
  echo =========================================
  echo.
exit /B 0

:doPrintHelp 
  echo.
  echo MNE-CPP testing script help.
  echo This script will run all applications in bin folder starting with test_*
  echo For help run: ./test_all.bat help
  echo Normal call has none or 2 arguments: ./test_all.bat [verbose] [withCoverage]
  echo.
  @REM call:doPrintConfiguration
exit /B 0

:success 
  echo [92m%~1[0m
exit /B 0

:fail
  echo [91m%~1[0m
  set /A "compoundOutput+=1"
exit /B 0

  :; # ########## WINDOWS SECTION ENDS ####################
  :; # ####################################################
  exit /b
BATCH

# ######################################################
# ############## LINUX MAC SECTION STARTS ##############
#!/bin/bash

#####  default parameters

VerboseMode="false"
ExitOnFirstFail="false"
RunCodeCoverage="false"

##### function definitions

doPrintConfiguration() {
  echo =========================================
  echo " "  VerboseMode = $VerboseMode
  echo " "  RunCodeCoverage = $RunCodeCoverage
  echo =========================================
}

doPrintHelp() {
  echo " "
  echo "MNE-CPP testing script help."
  echo "This script will run all applications in bin folder starting with test_*"
  echo "For help run: ./test_all.bat help"
  echo "Normal call has 2 or 3 arguments: ./test_all.bat [verbose] [withCoverage]"
  echo " "
}

## input arguments parsing

argc=$#
argv=("$@")

RunCodeCoverage="false"
VerboseMode="false"
BuildType="Release"


for (( j=0; j<argc; j++)); do
  if [ "${argv[j]}" == "verbose" ]; then
    BUILD_COMMAND=1
  elif [ "${argv[j]}" == "help" ]; then
    doPrintHelp="true"
    exit 1
  elif [ "${argv[j]}" == "withCoverage" ]; then
    RunCodeCoverage="true"
  elif [ "${argv[j]}" == "Debug" ]; then
    BuildType="Debug"
  elif [ "${argv[j]}" == "Release" ]; then
    BuildType="Release"
  fi
done

doPrintConfiguration

##########

RepoRootDir="$(dirname "$BASH_SOURCE")/../.."
echo $RepoRootDir

if [[ $(uname) == "Linux" ]]; then
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$RepoRootDir/lib
fi

########## start calling each test with some formatting
CompoundOutput=0
testColumnWidth=60
printf "%${testColumnWidth}s %s\n" " Test Name " " Result "

$RepoRootDir/out/${BuildType}/tests/test_mne_forward_solution


# for test in $RepoRootDir/out/${BuildType}/tests/test_*;
# do
#   # Run all tests and call gcov on all cpp files after each test run. Then upload to codecov for every test run.
#   # Codecov is able to process multiple uploads and merge them as soon as the CI job is done.
#   if [ $VerboseMode == "false" ]; then
#     $test &> /dev/null
#   else
#     $test 
#   fi
#   lastReturnValue=$?
# 
#   if [ $lastReturnValue -ne 0 ]; then 
#     CompoundOutput=$((CompoundOutput + 1))
#     printf "%${testColumnWidth}s \e[91m\033[1m %s \033[0m\e[0m\n" "${test}" "FAILED!"
#     if [ $ExitOnFirstFail == "true" ];
#     then
#       exit $lastReturnValue
#     fi
#   else
#     # echo ">> Test $test \t\t\t\t RockSolid!"
#     printf "%${testColumnWidth}s \e[92m %s \e[0m\n" "${test}" "Rock Solid!"
#   fi
# 
#   if [ $RunCodeCoverage == "true" ]; then
#     find ./libraries -type f -name "*.cpp" -exec gcov {} \; &> /dev/null
#     # Hide codecov output since it corrupts the log too much
#     ./codecov &> /dev/null
#   fi
# 
# done

# ############## LINUX MAC SECTION ENDS ################
# ######################################################

exit $CompoundOutput

