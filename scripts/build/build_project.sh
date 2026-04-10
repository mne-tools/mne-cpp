#!/usr/bin/env bash

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
BaseFolder="$(cleanAbsPath "$ScriptPath/../..")"
SourceFolder=""
BuildFolder=""
OutFolder=""
BuildType="Release"
BuildName="Release"
WithCodeCoverage="false"
CleanBuild="false"
BuildAll="false"
Rebuild="false"
NumProcesses="1"
MockBuild="false"
MockText=""
PrintHelp="false"
CMakeConfigFlags=""
ExtraArgs=""
ExtraSection=False
QtCustomPath=""
QtLinkage="dynamic"
EchoFlag=""

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
  echo ${EchoFlag} "\033[32m"
  echo ${EchoFlag} "\033[31m     *       )             (   (        "
  echo ${EchoFlag} "\033[31m   (  \`   (  (         (   )\ ))\ )     "
  echo ${EchoFlag} "\033[33m   )\))(  )\())(       )\ (()/(()/(     "
  echo ${EchoFlag} "\033[33m  ((_)()\((_)\ )\ ___(((_) /(_))(_))    "
  echo ${EchoFlag} "\033[37m  (_()((_)_((_|(_)___)\___(_))(_))      "
  echo ${EchoFlag} "\033[32m  |  \/  | \| | __| ((/ __| _ \ _ \     "
  echo "  | |\/| | .\` | _|   | (__|  _/  _/     "
  echo "  |_|  |_|_|\_|___|   \___|_| |_|       "
  echo "                                        "
  echo "  Build successful                      "
  echo ${EchoFlag} "\033[0m"
}

doShowBuildFailed() {
  echo ${EchoFlag} "\033[31m"
  echo "   _           _ _     _     __      _ _          _   "
  echo "  | |         (_) |   | |   / _|    (_) |        | |  "
  echo "  | |__  _   _ _| | __| |  | |_ __ _ _| | ___  __| |  "
  echo "  | '_ \| | | | | |/ _\` |  |  _/ _\` | | |/ _ \/ _\` |  "
  echo "  | |_) | |_| | | | (_| |  | || (_| | | |  __/ (_| |  "
  echo "  |_.__/ \__,_|_|_|\__,_|  |_| \__,_|_|_|\___|\__,_|  "
  echo "                                                      "
  echo "  Here we go...                                       "
  echo ${EchoFlag} "\033[0m"
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
  echo "Usage: ./scripts/build/build_project.sh [Options]"
  echo " "
  echo "All builds will be parallel."
  echo "All options can be used in undefined order, except for the extra args,"
  echo "which have to be at the end."
  echo " "
  echo "[help]  - Print this help."
  echo "[mock]  - Show commands do not execute them."
  echo "[all]   - Build entire project (libraries, applications, examples, tests)."
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
  echo "             If omitted, the script auto-detects src/external/qt/dynamic or"
  echo "             src/external/qt/static prepared by ./init.sh."
  echo "[--]       - mark beginning of extra-arguments section. any argument"
  echo "             following the double dash will be passed on to cmake"
  echo "             directly without it being parsed."
  echo "Environment overrides:"
  echo "  MNE_CPP_BUILD_JOBS / CMAKE_BUILD_PARALLEL_LEVEL can cap parallel jobs."
  echo " "
}

for (( j=0; j<argc; j++)); do
  if [ "${argv[j]}" == "coverage" ]; then
    WithCodeCoverage="true"
  elif [ "${argv[j]}" == "clean" ]; then
    CleanBuild="true"
  elif [ "${argv[j]}" == "all" ]; then
    BuildAll="true"
  elif [ "${argv[j]}" == "mock" ]; then
    MockBuild="true"
  elif [ "${argv[j]}" == "rebuild" ]; then
    Rebuild="true"
  elif [ "${argv[j]}" == "help" ]; then
    PrintHelp="true"
  elif [ "${argv[j]}" == "static" ]; then
    CMakeConfigFlags="${CMakeConfigFlags} -DBUILD_SHARED_LIBS=OFF"
    QtLinkage="static"
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

if [ -z "${QtCustomPath}" ]; then
  QtDefaultPath="${BaseFolder}/src/external/qt/${QtLinkage}"
  if [ -d "${QtDefaultPath}/bin" ]; then
    QtCustomPath="${QtDefaultPath}"
  fi
fi

if [ -n "${MNE_CPP_BUILD_JOBS:-}" ]; then
  NumProcesses="${MNE_CPP_BUILD_JOBS}"
elif [ -n "${CMAKE_BUILD_PARALLEL_LEVEL:-}" ]; then
  NumProcesses="${CMAKE_BUILD_PARALLEL_LEVEL}"
elif [ "$(uname)" == "Darwin" ]; then
  NumProcesses=$(sysctl -n hw.logicalcpu)
else
  NumProcesses=$(expr $(nproc --all))
  EchoFlag="-e"
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
  CMakeConfigFlags="${CMakeConfigFlags} -DCMAKE_PREFIX_PATH=${QtCustomPath}"

  if [ -d "${QtCustomPath}/lib/cmake/Qt6" ]; then
    CMakeConfigFlags="${CMakeConfigFlags} -DQt6_DIR=${QtCustomPath}/lib/cmake/Qt6 -DQT_DIR=${QtCustomPath}/lib/cmake/Qt6"
  elif [ -d "${QtCustomPath}/lib/cmake/Qt5" ]; then
    CMakeConfigFlags="${CMakeConfigFlags} -DQt5_DIR=${QtCustomPath}/lib/cmake/Qt5 -DQT_DIR=${QtCustomPath}/lib/cmake/Qt5"
  fi
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

if [ "${BuildAll}" == "true" ]; then
  echo "Building full project."
  CMakeConfigFlags="${CMakeConfigFlags} -DBUILD_ALL=ON"
fi

# On macOS, build GUI apps as .app bundles
if [[ "$(uname)" == "Darwin" ]]; then
  CMakeConfigFlags="${CMakeConfigFlags} -DBUILD_MAC_APP_BUNDLE=ON"
fi

if [ "${Rebuild}" == "false" ]; then
  echo " "
  echo "Configuring Build System:"
  ${MockText}cmake -B ${BuildFolder} -S ${SourceFolder} \
    -DCMAKE_BUILD_TYPE=${BuildType} \
    -DBINARY_OUTPUT_DIRECTORY=${OutFolder} \
    -DEIGEN_BUILD_CMAKE_PACKAGE=ON \
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
