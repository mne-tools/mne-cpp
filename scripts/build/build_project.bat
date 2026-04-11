@echo off
setlocal EnableDelayedExpansion

SET ScriptPath=%~dp0
SET BaseFolder=%ScriptPath%..\..

SET "EXIT_FAIL=1"
SET "EXIT_SUCCESS=0"
SET "EXIT_VALUE=3"

SET "SourceFolder="
SET "BuildFolder="
SET "OutFolder="

SET "MockBuild=False"
SET "MockText="

SET "CleanBuild=False"
SET "BuildAll=False"

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
SET "QtLinkage=dynamic"


:loop
IF NOT "%1"=="" (
  IF "%ExtraSection%"=="True" (
    REM cmd.exe splits on '=' so "-DVAR=VAL" arrives as two tokens.
    REM Reconstruct by peeking at the next arg when current starts with -D.
    SET "_ea=%~1"
    IF "!_ea:~0,2!"=="-D" (
      SET "ExtraArgs=!ExtraArgs! !_ea!=%~2"
      SHIFT
    ) ELSE (
      SET "ExtraArgs=!ExtraArgs! !_ea!"
    )
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
  IF "%ExtraSection%"=="False" IF "%1"=="all" (
    SET "BuildAll=True"
  )
  IF "%ExtraSection%"=="False" IF "%1"=="rebuild" (
    SET "Rebuild=True"
  )
  IF "%ExtraSection%"=="False" IF "%1"=="static" (
    SET "CMakeConfigFlags=!CMakeConfigFlags! -DBUILD_SHARED_LIBS=OFF"
    SET "QtLinkage=static"
  )
  IF "%ExtraSection%"=="False" IF "%1"=="tmsi" (
    SET "CMakeConfigFlags=!CMakeConfigFlags! -DWITH_TMSI=ON"
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

IF [%QtCustomPath%]==[] (
  SET "QtDefaultPath=%BaseFolder%\src\external\qt\%QtLinkage%"
  IF EXIST "!QtDefaultPath!\bin" (
    SET "QtCustomPath=!QtDefaultPath!"
  )
)

IF "%MockBuild%"=="True" (
    ECHO.
    ECHO Mock mode ON. Commands to be executed: 
    ECHO.
    SET "MockText=ECHO "
)

IF NOT [%QtCustomPath]==[] (
  SET "CMakeConfigFlags=!CMakeConfigFlags! -DCMAKE_PREFIX_PATH=!QtCustomPath!"

  IF EXIST "!QtCustomPath!\lib\cmake\Qt6\Qt6Config.cmake" (
    SET "CMakeConfigFlags=!CMakeConfigFlags! -DQt6_DIR=!QtCustomPath!\lib\cmake\Qt6 -DQT_DIR=!QtCustomPath!\lib\cmake\Qt6"
  ) ELSE IF EXIST "!QtCustomPath!\lib\cmake\Qt5\Qt5Config.cmake" (
    SET "CMakeConfigFlags=!CMakeConfigFlags! -DQt5_DIR=!QtCustomPath!\lib\cmake\Qt5 -DQT_DIR=!QtCustomPath!\lib\cmake\Qt5"
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

IF "%BuildAll%"=="True" (
  ECHO Building full project. 
  set "CMakeConfigFlags=!CMakeConfigFlags! -DBUILD_ALL=ON"
)

IF "%Rebuild%"=="False" (
    ECHO.
    ECHO Configuring build project
    %MockText%cmake -B %BuildFolder% -S %SourceFolder% -DCMAKE_BUILD_TYPE=%BuildType% -DBINARY_OUTPUT_DIRECTORY=%OutFolder% -DCMAKE_CXX_FLAGS="/EHsc /MP" -DEIGEN_BUILD_CMAKE_PACKAGE=ON %CMakeConfigFlags% %ExtraArgs%
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
  ECHO [all]   - Build entire project (libraries, applications, examples, tests).
  ECHO [clean] - Delete build and out folders for your configuration and exit.
  ECHO [Release*/Debug*] - Set the build type Debug/Release and name it.
  ECHO                     For example, Release_testA will build in release
  ECHO                     mode with a build folder /build/Release_testA
  ECHO                     and an out folder /out/Release_testA.
  ECHO [coverage] - Enable code coverage.
  ECHO [rebuild]  - Only rebuild existing build-system configuration.
  ECHO [static]   - Build project statically. QT_DIR and Qt5_DIR must be set to
  ECHO              point to a static version of Qt.
  ECHO [tmsi]     - Build tmsi plugin for mne-scan (needs SDK installed)
  ECHO [qt=\path\]- Use specified qt installation to build the project. This \path
  ECHO              must point to the directory containing the bin and lib folders
  ECHO              for the desired Qt version. ex. \some\path\to\Qt\5.15.2\msvc2019_64\
  ECHO              If omitted, the script auto-detects src\external\qt\dynamic or
  ECHO              src\external\qt\static prepared by init.bat.
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

