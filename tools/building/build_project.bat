:<<BATCH
    :;@echo off
    :; # ####################################################
    :; # ########## WINDOWS SECTION #########################
       

    SET ScriptPath=%~dp0
    SET BasePath=%ScriptPath%..\..

    cmake -B %BasePath%\build -S %BasePath%\src -DCMAKE_BUILD_TYPE=Release
    cmake --build %BasePath%\build --config Release

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

VerboseMode="false"
BuildType="Release"
WithCodeCoverage="false"
BuildFolder=""
SourceFolder=""
NumProcesses="1"

ScriptPath="$(readlink -f $(dirname "$0"))"
BasePath="$(readlink -f $ScriptPath/../..)"

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
# exit 1

echo cmake -B ${BuildFolder} -S ${SourceFolder} -DCMAKE_BUILD_TYPE=${BuildType} $CoverageOption
echo cmake --build ${BuildFolder} --parallel $NumProcesses

