:<<BATCH
    :;@echo off
    :; # ####################################################
    :; # ########## WINDOWS SECTION #########################
       

    SET ScriptPath=%~dp0
    SET BasePath=%ScriptPath%..\..

    SET "VerboseMode=False"
    SET "BuildType=Release"
    SET "WithCodeCoverage=False"
    SET "NumProcesses=1"
    
    SHIFT & SHIFT
    :loop
    IF NOT "%1"=="" (
      IF "%1"=="help" (
        goto :showHelp
      )
      IF "%1"=="Release" (
        SET "BuildType=Release"
        SHIFT
      )
      IF "%1"=="Debug" (
        SET "BuildType=Debug"
        SHIFT
      )
      IF "%1"=="coverage" (
        SET "WithCodeCoverage=true"
        SHIFT
      )

      SHIFT
      GOTO :loop
    )

    SET BuildFolder=%BasePath%\build\%BuildType%
    SET SrcFolder=%BasePath%\src

    call:doPrintConfiguration

    ECHO Cmake -B %BuildFolder% -S %BasePath%\src -DCMAKE_BUILD_TYPE=%BuildType%-DCMAKE_CXX_FLAGS="/MP"
    ECHO Cmake --build %BuildFolder% --config %BuildType%

    cmake -B %BuildFolder% -S %BasePath%\src -DCMAKE_BUILD_TYPE=%BuildType%-DCMAKE_CXX_FLAGS="/MP"
    cmake --build %BuildFolder% --config %BuildType%

    exit /B 

    :doPrintConfiguration
      ECHO.
      ECHO =========================================
      ECHO verbose = %VerboseMode%
      ECHO BuildType =%BuildType%
      ECHO Coverage =%WithCodeCoverage%
      ECHO BuildFolder  =%BuildFolder%
      ECHO SourceFolder =%SourceFolder%
      ECHO NumProcesses = %NumProcesses%
      ECHO .
      ECHO =========================================
      ECHO .
    exit /B 0

    :; # ########## WINDOWS SECTION ENDS ####################
    :; # ####################################################
    exit /b
BATCH
# ######################################################
# ############## LINUX MAC SECTION STARTS ##############
#!/bin/bash

#####  default parameters

argc=$#
argv=("$@")

function cleanAbsPath()
{
    local  cleanAbsPathStr="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$cleanAbsPathStr"
}

VerboseMode="false"
BuildType="Release"
WithCodeCoverage="false"
BuildFolder=""
SourceFolder=""
NumProcesses="1"

ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BasePath="$(cleanAbsPath "$ScriptPath/../..")"

if [ "$(uname)" == "Darwin" ]; then
    NumProcesses=$(sysctl -n hw.physicalcpu)
else 
    NumProcesses=$(expr $(nproc --all) / 2)
fi

doPrintConfiguration() {
  echo " "
  echo ========================================================================
  echo ======================== MNE-CPP BUILD CONFIG ==========================
  echo " "
  echo " ScriptPath   = $ScriptPath"
  echo " BasePath     = $BasePath"
  echo " BuildFolder  = $BuildFolder"
  echo " SourceFolder = $SourceFolder"
  echo " "
  echo " VerboseMode = $VerboseMode"
  echo " Buildtype = $BuildType"
  echo " WithCodeCoverage = $WithCodeCoverage"
  echo " NumProcesses = $NumProcesses"
  echo " "
  echo ========================================================================
  echo ========================================================================
  echo " "
}

doPrintHelp() {
  echo " "
  echo "MNE-CPP building script help."
  echo ""
  echo "Usage: ./build_project.bat [verbose] [(Release)/Debug] [coverage]"
  echo ""
}

for (( j=0; j<argc; j++)); do
  if [ "${argv[j]}" == "verbose" ]; then
    VerboseMode="true"
  elif [ "${argv[j]}" == "coverage" ]; then
    WithCodeCoverage="true"
  elif [ "${argv[j]}" == "Debug" ]; then
    BuildType="Debug"
  elif [ "${argv[j]}" == "Release" ]; then
    BuildType="Release"
  elif [ "${argv[j]}" == "help" ]; then
    doPrintHelp
    exit 1
  fi
done

if [ "$WithCodeCoverage" == "false"  ]; then
  CoverageOption=""
elif [ "$WithCodeCoverage" == "true"  ]; then
  CoverageOption="-DWITH_CODE_COV=ON"
fi

BuildFolder=${BasePath}/build/${BuildType}
SourceFolder=${BasePath}/src

doPrintConfiguration

cmake -B ${BuildFolder} -S ${SourceFolder} -DCMAKE_BUILD_TYPE=${BuildType} ${CoverageOption}
cmake --build ${BuildFolder} --parallel $NumProcesses

exit 0
