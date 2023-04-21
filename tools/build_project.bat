:<<BATCH
    @echo off
    :; # ####################################################
    :; # ########## WINDOWS SECTION #########################
       
    setlocal EnableDelayedExpansion

    SET ScriptPath=%~dp0
    SET BaseFolder=%ScriptPath%..

    SET "EXIT_FAIL=1"
    SET "EXIT_SUCCESS=0"
    SET "EXIT_VALUE=3"

    SET "SourceFolder="
    SET "BuildFolder="
    SET "OutFolder="

    SET "MockBuild=False"
    SET "MockText="

    SET "CleanBuild=False"

    SET "VerboseMode=False"
    SET "BuildType=Release"
    SET "BuildName=Release"

    SET "WithCodeCoverage=False"
    SET "NumProcesses=1"

    SET "Rebuild=False"
    SET "CMakeConfigFlags="
    SET "ExtraArgs="
    SET "ExtraSection=False"
    SET "QtCustomPath="


    :loop
    IF NOT "%1"=="" (
      IF "%ExtraSection%"=="True" (
        SET "ExtraArgs=!ExtraArgs! %~1"
      )
      IF "%ExtraSection%"=="False" IF "%1"=="help" (
        call:showLogo
        call:showHelp
        goto :endOfScript
      )
      set Arg=%1

      IF "%ExtraSection%"=="False" IF NOT x!Arg!==x!Arg:Release=! (
        SET BuildType=Release
        SET BuildName=!Arg!
      )
      IF "%ExtraSection%"=="False" IF NOT x!Arg!==x!Arg:Debug=! (
        SET BuildType=Debug
        SET BuildName=!Arg!
      )
      IF "%ExtraSection%"=="False" IF "%1"=="coverage" (
        SET "WithCodeCoverage=True"
      )
      IF "%ExtraSection%"=="False" IF "%1"=="mock" (
        SET "MockBuild=True"
      )
      IF "%ExtraSection%"=="False" IF "%1"=="clean" (
        SET "CleanBuild=True"
      )
      IF "%ExtraSection%"=="False" IF "%1"=="rebuild" (
        SET "Rebuild=True"
      )
      IF "%ExtraSection%"=="False" IF "%1"=="static" (
        SET "CMakeConfigFlags=!CMakeConfigFlags! -DBUILD_SHARED_LIBS=OFF"
      )
      IF "%ExtraSection%"=="False" IF "%1"=="--" (
        SET "ExtraSection=True"
      )
      IF "%1"=="qt" (
        IF NOT "%2"=="" (
            SET "QtCustomPath=%2"
            SHIFT
        )
      )
      SHIFT
      GOTO :loop
    )

    SET SourceFolder=%BaseFolder%\src
    SET BuildFolder=%BaseFolder%\build\%BuildName%
    SET OutFolder=%BaseFolder%\out\%BuildName%

    IF "%MockBuild%"=="True" (
        ECHO.
        ECHO Mock mode ON. Commands to be executed: 
        ECHO.
        SET "MockText=ECHO "
    )

    IF NOT [%QtCustomPath]==[] (
      FOR /D %%s in (!QtCustomPath!\lib\cmake\*) do (

        set LIB_NAME=%%~ns
        set "CMakeConfigFlags=!CMakeConfigFlags! -D!LIB_NAME!_DIR=%%s"

        IF "!LIB_NAME!"=="Qt5" (
          set "CMakeConfigFlags=!CMakeConfigFlags! -DQT_DIR=%%s"
        )
        IF "!LIB_NAME!"=="Qt6" (
          set "CMakeConfigFlags=!CMakeConfigFlags! -DQT_DIR=%%s"
        )
      )
    )

    call:showLogo
    call:doPrintConfiguration
    
    IF "%CleanBuild%"=="True" (
        ECHO Deleting folders: 
        ECHO   %BuildFolder%
        ECHO   %OutFolder%
        ECHO.

        %MockText%RMDIR /S /Q %BuildFolder%
        %MockText%RMDIR /S /Q %OutFolder%

        goto :endOfScript
    )

    IF "%Rebuild%"=="False" (
        ECHO.
        ECHO Configuring build project
        %MockText%cmake -B %BuildFolder% -S %SourceFolder% -DCMAKE_BUILD_TYPE=%BuildType% -DBINARY_OUTPUT_DIRECTORY=%OutFolder% -DCMAKE_CXX_FLAGS="/MP" %CMakeConfigFlags% %ExtraArgs%
    )

    %MockText%cmake --build %BuildFolder% --config %BuildType% && call:buildSuccessful || call:buildFailed

    :endOfScript

    exit /B %EXIT_VALUE%

    :buildSuccessful
      SET "EXIT_VALUE=%EXIT_SUCCESS%"
      call:showBuildSuccessful
    exit /B 0

    :buildFailed
      SET "EXIT_VALUE=%EXIT_FAIL%"    
      call:showBuildFailed
    exit /B 0

    :doPrintConfiguration
      ECHO.
      ECHO ====================================================================
      ECHO ===================== MNE-CPP BUILD CONFIG =========================
      ECHO.
      ECHO ScriptPath   = %ScriptPath%
      ECHO BaseFolder   = %BaseFolder%
      ECHO SourceFolder = %SourceFolder%
      ECHO BuildFolder  = %BuildFolder%
      ECHO OutFolder    = %OutFolder%
      ECHO.
      ECHO VerboseMode  = %VerboseMode%
      ECHO MockBuild    = %MockBuild%
      ECHO CleanBuild   = %CleanBuild%
      ECHO BuildType    = %BuildType%
      ECHO BuildName    = %BuildName%
      ECHO Rebuild      = %Rebuild%
      ECHO QtCustomPath = %QtCustomPath%
      ECHO Coverage     = %WithCodeCoverage%
      ECHO NumProcesses = %NumProcesses%
      ECHO CMakeConfigFlags = %CMakeConfigFlags%
      ECHO ExtraArgs    = %ExtraArgs%
      ECHO.
      ECHO ====================================================================
      ECHO ====================================================================
      ECHO.
    exit /B 0

    :showHelp
      ECHO. 
      ECHO MNE-CPP building script help.
      ECHO. 
      ECHO Usage: ./build_project.bat [Options]
      ECHO.
      ECHO All builds will be parallel.
      ECHO All options can be used in undefined order, except for the extra args, 
      ECHO which have to be at the end.
      ECHO.
      ECHO [help]  - Print this help.
      ECHO [mock]  - Show commands do not execute them.
      ECHO [clean] - Delete build and out folders for your configuration and exit.
      ECHO [Release*/Debug*] - Set the build type Debug/Release and name it.
      ECHO                     For example, Release_testA will build in release
      ECHO                     mode with a build folder /build/Release_testA
      ECHO                     and an out folder /out/Release_testA.
      ECHO [coverage] - Enable code coverage.
      ECHO [rebuild]  - Only rebuild existing build-system configuration.
      ECHO [static]   - Build project statically. QT_DIR and Qt5_DIR must be set to
      ECHO              point to a static version of Qt.
      ECHO [qt=\path\]- Use specified qt installation to build the project. This \path
      ECHO              must point to the directory containing the bin and lib folders
      ECHO              for the desired Qt version. ex. \some\path\to\Qt\5.15.2\msvc2019_64\
      ECHO [--]       - Mark beginning of extra-arguments section. Any argument
      ECHO              following the double dash will be passed on to cmake 
      ECHO              directly without it being parsed.      
      ECHO.
    exit /B 0

    :showLogo
      ECHO.
      ECHO.
      ECHO     _    _ _  _ ___     ___ __  ___   
      ECHO    ^|  \/  ^| \^| ^| __^|   / __^| _ \ _ \  
      ECHO    ^| ^|\/^| ^| .\ ^| _^|   ^| (__^|  _/  _/  
      ECHO    ^|_^|  ^|_^|_^|\_^|___^|   \___^|_^| ^|_^|    
      ECHO.
      ECHO    Build tool                         
      ECHO.
    exit /B 0

    :showBuildSuccessful
      ECHO.
      ECHO.
      ECHO      *       )             (   (        
      ECHO    (  \`   (  (         (   )\ ))\ )    
      ECHO    )\))(  )\())(       )\ (()/(()/(     
      ECHO   ((_)()\((_)\ )\ ___(((_) /(_))(_))    
      ECHO   (_()((_)_((_^|(_)___)\___(_))(_))      
      ECHO   ^|  \/  ^| \^| ^| __^| ((/ __^| _ \ _ \     
      ECHO   ^| ^|\/^| ^| .\`^| _^|   ^| (__^|  _/  _/     
      ECHO   ^|_^|  ^|_^|_^|\_^|___^|   \___^|_^| ^|_^|       
      ECHO.
      ECHO   Build successful                      
      ECHO.
    exit /B 0

    :showBuildFailed
      ECHO.
      ECHO.
      ECHO    _           _ _     _     __      _ _          _   
      ECHO   ^| ^|         (_) ^|   ^| ^|   / _^|    (_) ^|        ^| ^|  
      ECHO   ^| ^|__  _   _ _^| ^| __^| ^|  ^| ^|_ __,_ _^| ^| ___  __^| ^|  
      ECHO   ^|  _ \^| ^| ^| ^| ^| ^|/ _  ^|  ^|  _/ _  ^| ^| ^|/ _ \/ _  ^|     
      ECHO   ^| ^|_) ^| ^|_^| ^| ^| ^| (_^| ^|  ^| ^|^| (_^| ^| ^| ^|  __/ (_^| ^|  
      ECHO   ^|_.__/ \__,_^|_^|_^|\__,_^|  ^|_^| \__,_^|_^|_^|\___^|\__,_^|  
      ECHO.
      ECHO   Here we go...                                       
      ECHO.
    exit /B 1

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


EXIT_FAIL=1
EXIT_SUCCESS=0

ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BaseFolder="$(cleanAbsPath "$ScriptPath/..")"
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
MockText=""
PrintHelp="false"
CMakeConfigFlags=""
ExtraArgs=""
ExtraSection=False
QtCustomPath=""
ChillMode="false"

doShowLogo() {
  echo "                                      "
  echo "    _    _ _  _ ___     ___ __  ___   "
  echo "   |  \/  | \| | __|   / __| _ \ _ \  "
  echo "   | |\/| |  \` | _|   | (__|  _/  _/  "
  echo "   |_|  |_|_|\_|___|   \___|_| |_|    "
  echo "                                      "
  echo "   Build tool                         "
  echo "                                      "
}

doShowLogoFlames() {
  echo "                                        "
  echo "     *       )             (   (        "
  echo "   (  \`   (  (         (   )\ ))\ )     "
  echo "   )\))(  )\())(       )\ (()/(()/(     "
  echo "  ((_)()\((_)\ )\ ___(((_) /(_))(_))    "
  echo "  (_()((_)_((_|(_)___)\___(_))(_))      "
  echo "  |  \/  | \| | __| ((/ __| _ \ _ \     "
  echo "  | |\/| | .\` | _|   | (__|  _/  _/     "
  echo "  |_|  |_|_|\_|___|   \___|_| |_|       "
  echo "                                        "
  echo "  Build successful                      "
  echo "                                        "
}

doShowBuildFailed() {
  echo "                                                      "
  echo "   _           _ _     _     __      _ _          _   "
  echo "  | |         (_) |   | |   / _|    (_) |        | |  "
  echo "  | |__  _   _ _| | __| |  | |_ __ _ _| | ___  __| |  "
  echo "  | '_ \| | | | | |/ _\` |  |  _/ _\` | | |/ _ \/ _\` |  "
  echo "  | |_) | |_| | | | (_| |  | || (_| | | |  __/ (_| |  "
  echo "  |_.__/ \__,_|_|_|\__,_|  |_| \__,_|_|_|\___|\__,_|  "
  echo "                                                      "
  echo "  Here we go...                                       "
  echo "                                                      "
}

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
  echo " QtCustomPath = $QtCustomPath"
  echo " WithCodeCoverage = $WithCodeCoverage"
  echo " NumProcesses = $NumProcesses"
  echo " MockBuild = $MockBuild"
  echo " MockText = $MockText"
  echo " PrintHelp = $PrintHelp"
  echo " ExtraArgs = $ExtraArgs"
  echo " "
  echo ========================================================================
  echo ========================================================================
  echo " "
}

doPrintHelp() {
  echo " "
  echo "Usage: ./build_project.bat [Options]"
  echo " "
  echo "All builds will be parallel."
  echo "All options can be used in undefined order, except for the extra args," 
  echo "which have to be at the end."
  echo " "
  echo "[help]  - Print this help."
  echo "[mock]  - Show commands do not execute them."
  echo "[clean] - Delete build and out folders for your configuration and exit."
  echo "[Release*/Debug*] - Set the build type (Debug/Release) and name it."
  echo "                    For example, Release_testA will build in release"
  echo "                    mode with a build folder /build/Release_testA"
  echo "                    and an out folder /out/Release_testA."
  echo "[coverage] - Enable code coverage."
  echo "[rebuild]  - Only rebuild existing build-system configuration."
  echo "[static]   - Build project statically. QT_DIR and Qt5_DIR must be set to"
  echo "             point to a static version of Qt."
  echo "[qt=<path>]- Use specified qt installation to build the project. This path"
  echo "             must point to the directory containing the bin and lib folders"
  echo "             for the desired Qt version. ex. /some/path/to/Qt/5.15.2/gcc_64/"
  echo "[--]       - mark beginning of extra-arguments section. any argument"
  echo "             following the double dash will be passed on to cmake"
  echo "             directly without it being parsed."
  echo " "
}

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
    PrintHelp="true"
  elif [ "${argv[j]}" == "static" ]; then
    CMakeConfigFlags="${CMakeConfigFlags} -DBUILD_SHARED_LIBS=OFF"
  fi
  IFS='=' read -r -a inkarg <<< "${argv[j]}"
  if [ "${inkarg[0]}" == "qt" ]; then
      QtCustomPath="${inkarg[1]}"
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
  if [ "${argv[j]}" == "--" ]; then
    for ((j++ ; j<argc; j++)); do
      ExtraArgs="${ExtraArgs} ${argv[j]}"
    done
  fi
done

if [ "$WithCodeCoverage" == "false"  ]; then
  CoverageOption=""
elif [ "$WithCodeCoverage" == "true"  ]; then
  CoverageOption="-DWITH_CODE_COV=ON"
fi

SourceFolder=${BaseFolder}/src
BuildFolder=${BaseFolder}/build/${BuildName}
OutFolder=${BaseFolder}/out/${BuildName}

if [ "$(uname)" == "Darwin" ]; then
  NumProcesses=$(sysctl -n hw.logicalcpu)
else
  NumProcesses=$(expr $(nproc --all))
fi

#### command execution starts here

doShowLogo

if [ "${PrintHelp}" == "true" ]; then
  doPrintHelp
  exit ${EXIT_SUCCESS}
fi

if [ "${MockBuild}" == "true" ]; then
  MockText="echo "
  echo " "
  echo "Mock mode ON. Commands to be executed: "
  echo " "
else
  MockText=""
fi

if [ -n "${QtCustomPath}" ]; then
  
  for d in ${QtCustomPath}/lib/cmake/* ; do
    d=$(basename ${d})
    QT_CMAKE_FLAGS="${QT_CMAKE_FLAGS}
      -D${d}_DIR=${QtCustomPath}/lib/cmake/${d}"
    if [ "$d" == "Qt5" ]; then
      QT_CMAKE_FLAGS="${QT_CMAKE_FLAGS}
        -DQT_DIR=${QtCustomPath}/lib/cmake/${d}"
    elif [ "$d" == "Qt6" ]; then
      QT_CMAKE_FLAGS="${QT_CMAKE_FLAGS}
        -DQT_DIR=${QtCustomPath}/lib/cmake/${d}"
    fi
  done

  CMakeConfigFlags="${CMakeConfigFlags} ${QT_CMAKE_FLAGS}"
fi

doPrintConfiguration

if [ "${CleanBuild}" == "true" ]; then
  echo "Deleting folders: "
  echo "  ${BuildFolder}"
  echo "  ${OutFolder}"
  echo " "
  ${MockText}rm -fr ${BuildFolder}
  ${MockText}rm -fr ${OutFolder}
  exit ${EXIT_SUCCESS}
fi

if [ "${Rebuild}" == "false" ]; then
  echo " "
  echo "Configuring Build System:" 
  ${MockText}cmake -B ${BuildFolder} -S ${SourceFolder} \
    -DCMAKE_BUILD_TYPE=${BuildType} \
    -DBINARY_OUTPUT_DIRECTORY=${OutFolder} \
    ${CoverageOption} \
    ${CMakeConfigFlags} \
    ${ExtraArgs}
fi

echo " "
echo "Compiling:"
${MockText}cmake --build ${BuildFolder} --parallel $NumProcesses
if [ $? -eq 0 ]; then
  BuildSuccessful="true"
else
  BuildSuccessful="false"
fi

echo " "
echo " "
echo "Copy compile_commands.json to root folder."
echo " "
${MockText}cp ${BuildFolder}/compile_commands.json ${BaseFolder}
echo " "
echo " "

if [ "${BuildSuccessful}" == "true" ]; then
  doShowLogoFlames
  exit ${EXIT_SUCCESS}
else
  doShowBuildFailed
  exit ${EXIT_FAIL}
fi

exit ${EXIT_FAIL}

