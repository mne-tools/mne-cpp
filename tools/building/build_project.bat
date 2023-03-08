:<<BATCH
    @echo off
    :; # ####################################################
    :; # ########## WINDOWS SECTION #########################
       
  
    setlocal EnableDelayedExpansion


    SET ScriptPath=%~dp0
    SET BaseFolder=%ScriptPath%..\..

    SET SourceFolder=
    SET BuildFolder=
    SET OutFolder=

    SET "MockBuild=False"
    SET MockText=

    SET "CleanBuild=False"

    SET "VerboseMode=False"
    SET "BuildType=Release"
    SET "BuildName=Release"

    SET "WithCodeCoverage=False"
    SET "NumProcesses=1"
    
    :loop
    IF NOT "%1"=="" (
      ECHO 1 is -%1-
      IF "%1"=="help" (
        goto :showHelp
      )
      set Arg=%1
      ECHO ARG is -!Arg!-

      ECHO DDD !Arg! !Arg:Debug=!
      echo RRR !Arg! !Arg:Release=!

      IF NOT x!Arg!==x!Arg:Release=! (
        SET BuildType=Release
        SET BuildName=!Arg!
      )
      IF NOT x!Arg!==x!Arg:Debug=! (
        SET BuildType=Debug
        SET BuildName=!Arg!
      )
      IF "%1"=="coverage" (
        SET "WithCodeCoverage=True"
      )
      IF "%1"=="mock" (
        SET "MockBuild=True"
      )
      IF "%1"=="clean" (
        SET "CleanBuild=True"
      )

      SHIFT
      GOTO :loop
    )

    SET SourceFolder=%BaseFolder%\src
    SET BuildFolder=%BaseFolder%\build\%BuildName%
    SET OutFolder=%BaseFolder%\out\%BuildName%

    call:doPrintConfiguration

    IF "%MockBuild%"=="True" (
        ECHO .
        ECHO Mock mode ON. Commands to be executed: 
        ECHO .
        SET "MockText=ECHO "
    )
    
    IF "%CleanBuild%"=="True" (
        echo Deleting folders: 
        echo   %BuildFolder%
        echo   %OutFolder%
        echo .

        RMDIR /S /Q %BuildFolder%
        RMDIR /S /Q %OutFolder%
    )

    %MockText%cmake -B %BuildFolder% -S %SourceFolder% -DCMAKE_BUILD_TYPE=%BuildType% -DBINARY_OUTPUT_DIRECTORY=%OutFolder% -DCMAKE_CXX_FLAGS="/MP"
    %MockText%cmake --build %BuildFolder% --config %BuildType%

    exit /B 

    :doPrintConfiguration
      ECHO .
      ECHO ====================================================================
      ECHO ===================== MNE-CPP BUILD CONFIG =========================
      ECHO .
      ECHO ScriptPath   = %ScriptPath%
      ECHO BaseFolder   = %BaseFolder%
      ECHO SourceFolder = %SourceFolder%
      ECHO BuildFolder  = %BuildFolder%
      ECHO OutFolder    = %OutFolder%
      ECHO .
      ECHO BuildType    = %BuildType%
      ECHO BuildName    = %BuildName%
      ECHO CleanBuild   = %CleanBuild%
      ECHO Coverage =%WithCodeCoverage%
      ECHO BuildFolder  =%BuildFolder%
      ECHO SourceFolder =%SourceFolder%
      ECHO NumProcesses = %NumProcesses%
      ECHO MockBuild = %MockBuild%
      ECHO .
      ECHO ====================================================================
      ECHO ====================================================================
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

ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BaseFolder="$(cleanAbsPath "$ScriptPath/../..")"
SourceFolder=""
BuildFolder=""
OutFolder=""
BuildType="Release"
BuildName="Release"
WithCodeCoverage="false"
CleanBuild="false"
Rebuild="false"
NumProcesses="1"
MockBuild="false"

if [ "$(uname)" == "Darwin" ]; then
    NumProcesses=$(sysctl -n hw.logicalcpu)
else 
    NumProcesses=$(expr $(nproc --all))
fi

doPrintConfiguration() {
  echo " "
  echo ========================================================================
  echo ======================== MNE-CPP BUILD CONFIG ==========================
  echo " "
  echo " ScriptPath   = $ScriptPath"
  echo " BaseFolder   = $BaseFolder"
  echo " SourceFolder = $SourceFolder"
  echo " BuildFolder  = $BuildFolder"
  echo " OutFolder    = $OutFolder"
  echo " "
  echo " Buildtype = $BuildType"
  echo " BuildName = $BuildName"
  echo " CleanBuild = $CleanBuild"
  echo " Rebuild = $Rebuild"
  echo " WithCodeCoverage = $WithCodeCoverage"
  echo " NumProcesses = $NumProcesses"
  echo " MockBuild = $MockBuild"
  echo " "
  echo ========================================================================
  echo ========================================================================
  echo " "
}

doPrintHelp() {
  echo " "
  echo "MNE-CPP building script help."
  echo " "
  echo "Usage: ./build_project.bat [Options]"
  echo " "
  echo "All builds will be parallel."
  echo "All options can be used in undefined order."
  echo " "
  echo "[help] - Print this help."
  echo "[mock] - Show commands do not execute them."
  echo "[clean] - Delete build and out folders for your configuration and exit."
  echo "[Release*/Debug*] - Set the build type (Debug/Release) and name it."
  echo "                    For example, Release_testA will build in release"
  echo "                    mode with a build folder /build/Release_testA"
  echo "                    and an out folder /out/Release_testA."
  echo "[coverage] -  Enable code coverage."
  echo "[rebuild] - Only rebuild existing build-system configuration."
  echo " "
}

SubStrDebug="Debug"
SubStrRelease="Release"
for (( j=0; j<argc; j++)); do
  if [ "${argv[j]}" == "coverage" ]; then
    WithCodeCoverage="true"
  elif [ "${argv[j]}" == "clean" ]; then
    CleanBuild="true"
  elif [ "${argv[j]}" == "mock" ]; then
    MockBuild="true"
  elif [ "${argv[j]}" == "rebuild" ]; then
    Rebuild="true"
  elif [ "${argv[j]}" == "help" ]; then
    doPrintHelp
    exit 1
  fi
  case ${argv[j]} in
    *"Debug"*)
      BuildType="Debug"
      BuildName="${argv[j]}"
      ;;
    *"Release"*)
      BuildType="Release"
      BuildName="${argv[j]}"
      ;;
  esac
done

if [ "$WithCodeCoverage" == "false"  ]; then
  CoverageOption=""
elif [ "$WithCodeCoverage" == "true"  ]; then
  CoverageOption="-DWITH_CODE_COV=ON"
fi

SourceFolder=${BaseFolder}/src
BuildFolder=${BaseFolder}/build/${BuildName}
OutFolder=${BaseFolder}/out/${BuildName}

doPrintConfiguration

if [ "${MockBuild}" == "true" ]; then
  MockText="echo "
  echo " "
  echo "Mock mode ON. Commands to be executed: "
  echo " "
else
  MockText=""
fi

if [ "${CleanBuild}" == "true" ]; then
  echo "Deleting folders: "
  echo "  ${BuildFolder}"
  echo "  ${OutFolder}"
  echo " "
  ${MockText}rm -fr ${BuildFolder}
  ${MockText}rm -fr ${OutFolder}
  exit 0
fi

if [ "${Rebuild}" == "false" ]; then
  echo " "
  echo "Configuring build project:" 
  ${MockText}cmake -B ${BuildFolder} -S ${SourceFolder} -DCMAKE_BUILD_TYPE=${BuildType} -DBINARY_OUTPUT_DIRECTORY=${OutFolder} ${CoverageOption}
fi

echo " "
echo "Compiling:"
${MockText}cmake --build ${BuildFolder} --parallel $NumProcesses

echo "Copy compile_commands.json file to root folder."
${MockText}cp -v ${BuildFolder}/compile_commands.json ${BaseFolder}

exit 0

