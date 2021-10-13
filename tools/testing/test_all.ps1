:;# This script performs generates and copies the necesary library dependencies for running qt-projects both for 
:;# dynamic and for staic builds. 
:;#
:;# This file is part of the MNE-CPP project. For more information visit: https://mne-cpp.github.io/
:;#
:;# This script is based ßon an open-source cross-platform script template.
:;# For more information you can visit: https://github.com/juangpc/multiplatform_bash_cmd
:;# 

:<<BATCH
:;@echo off
:; # ########## WINDOWS SECTION #########################

Set-StrictMode -Version Latest
$ErrorActionPreference = "SilentlyContinue"
$PrintOutput = "False"
$StopOnFirstTestFail = "False"

$verboseMode=$args[0]
$exitMode=$args[1]

if ( $verboseMode -eq "verbose" ) {
  $PrintOutput = "True"
}

if ( $exitMode -eq "exitOnFail" ) {
  $StopOnFirstTestFail = "True"
}

$global:CompoundOutput = 0

function compoundReturnValue( $e )
{
  if( $e -ne 0)
  {
    $global:CompoundOutput = $global:CompoundOutput + 1;
  }
  # Write-Host "compound output: $global:CompoundOutput";
}

$CURRENT_PATH = pwd
cd $PSScriptRoot/../..

Write-Host "" ""

Get-ChildItem -Filter bin/test_*.exe | ForEach {
  if ( $PrintOutput -eq "True" ) {
    &$_.FullName;
  } else {
    Write-Host " $_  => " -NoNewline
    # ($out = &$_.Fullname) ;
    ($out = &$_.Fullname) 2>&1 | out-null;
    # &$_.Fullname 2>&1 | Out-Null;
  }
  compoundReturnValue $lastexitcode;
  if ( $lastexitcode -ne 0 ) { 
    Write-Host " FAIL!"  -ForegroundColor Red;
    if ($StopOnFirstTestFail -eq "True" ) {
      exit $lastexitcode
    }
  }  else { 
    Write-Host " RockSolid!" -ForegroundColor Green;
  }
}

cd $CURRENT_PATH

exit $CompoundOutput;

:; # ########## WINDOWS SECTION ENDS ####################
:; # ####################################################
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
  echo " "  ExitOnFirstFail = $ExitOnFirstFail
  echo " "  RunCodeCoverage = $RunCodeCoverage
  echo =========================================
}

doPrintHelp() {
  echo " "
  echo "MNE-CPP testing script help."
  echo "This script will run all applications in bin folder starting with "test_""
  echo "For help run: ./test_all help"
  echo "Normal call has 2 or 3 arguments: ./test_all (verbose/summary) (exitOnFail/continueOnFail) [coverage/noCoverage]"
  echo " "
}

## input arguments parsing

if [[ -z "$1" ]]; then #IF first argument is missing
  echo "Running script in default mode."
  echo " "
else
  if [[ $1 == verbose ]]; then #if first argument is equal to
    VerboseMode="true"
  elif [[ $1 == summary ]]; then #if second argument is equal to
    VerboseMode="false"
  elif [[ $1 == help ]]; then #if third argument is equal to
    doPrintHelp
    exit 1
  fi

  if [[ $2 == exitOnFail ]]; then
    ExitOnFirstFail="true"
  elif [[ $2 == continueOnFail ]]; then
    ExitOnFirstFail="false"    
  fi

  if [[ $3 == coverage ]]; then
    RunCodeCoverage="true"
  else 
    RunCodeCoverage="false"
  fi
fi

doPrintConfiguration  # call to this function defined previously

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

for test in $RepoRootDir/bin/test_*;
do
  # Run all tests and call gcov on all cpp files after each test run. Then upload to codecov for every test run.
  # Codecov is able to process multiple uploads and merge them as soon as the CI job is done.
  if [ $VerboseMode == "false" ];
  then
    $test &> /dev/null
    lastReturnValue=$?
  else
    $test 
    lastReturnValue=$?
  fi

  if [ $lastReturnValue -ne 0 ];
  then 
    CompoundOutput=$((CompoundOutput + 1))
    printf "%${testColumnWidth}s \e[91m\033[1m %s \033[0m\e[0m\n" "${test}" "FAILED!"
    if [ $ExitOnFirstFail == "true" ];
    then
      exit $lastReturnValue
    fi
  else
    # echo ">> Test $test \t\t\t\t RockSolid!"
    printf "%${testColumnWidth}s \e[92m %s \e[0m\n" "${test}" "Rock Solid!"
  fi

  if [ $RunCodeCoverage == "true" ];
  then
    find ./libraries -type f -name "*.cpp" -exec gcov {} \; &> /dev/null
    # Hide codecov output since it corrupts the log too much
    codecov &> /dev/null
  fi

done

exit $CompoundOutput

# ############## LINUX MAC SECTION ENDS ################
# ######################################################



