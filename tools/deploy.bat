:;# This script performs generates and copies the necesary library dependencies for running qt-projects both for 
:;# dynamic and for staic builds. 
:;#
:;# This file is part of the MNE-CPP project. For more information visit: https://mne-cpp.github.io/
:;#
:;# This script is based on an open-source cross-platform script template.
:;# For more information you can visit: https://github.com/juangpc/multiplatform_bash_cmd
:;#

:<<BATCH
    :;@echo off
    :; # ########## WINDOWS SECTION #########################

    SET SCRIPT_PATH=%~dp0
    SET BASE_PATH=%SCRIPT_PATH%..
    SET LINK_OPTION=%1
    SET PACK_OPTION=%2
    SET OUT_DIR_NAME=%3
    SET BUILD_NAME=Release\
    
    SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\"

    IF "%LINK_OPTION%"=="" (
        SET LINK_OPTION=dynamic
    )

    IF "%OUT_DIR_NAME%"=="" (
        SET OUT_DIR_NAME=%BASE_PATH%\out\Release
    )

    IF "%LINK_OPTION%"=="dynamic" (
        
        cd %OUT_DIR_NAME%\apps

        for /f %%f in ('dir *.dll /s /b') do (
          windeployqt %%f
        )

        for /f %%f in ('dir *.exe /s /b') do (
            windwployqt %%f
        )

        REM Solve dependencies for libraries
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_utils.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_fiff.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_fs.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_events.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_mne.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_fwd.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_inverse.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_communication.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_rtprocessing.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_connectivity.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_disp.dll
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_disp3D.dll

        REM solve dependencies for applications
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_analyze
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_scan
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_anonymize
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_rt_server
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_forward_solution
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_edf2fiff
        REM windeployqt %BASE_PATH%\out\Release\bin\mne_dipole_fit

        REM solve dependencies for tests 
        REM windeployqt %BASE_PATH%\out\Release\bin\test_coregistration.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_dipole_fit.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_edf2fiff_rwr.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_fiff_coord_trans.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_fiff_cov.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_fiff_digitizer.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_fiff_mne_types_io.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_fiff_rwr.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_filtering.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_geometryinfo.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_hpiDataUpdater.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_hpiFit.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_hpiFit_integration.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_hpiModelParameter.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_interpolation.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_mne_anonymize.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_mne_forward_solution.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_mne_msh_display_surface_set.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_mne_project_to_surface.ex
        REM windeployqt %BASE_PATH%\out\Release\bin\test_sensorSet.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_signalModel.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_spectral_connectivity.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\test_utils_circularbuffer.exe

        REM solve dependencies with test
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_averaging.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_cancel_noise.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_clustered_inverse_mne.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_clustered_inverse_mne_raw.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_clustered_inverse_pwl_rap_music_raw.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_clustered_inverse_rap_music_raw.exe    
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_compute_forward.pro.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_connectivity.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_connectivity_comparison.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_connectivity_performace.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_coreg.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_disp.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_disp3D.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_evoked_grad_amp.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_fiff.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_fiff_sniffer.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_file_utils.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_filtering.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_find_Evoked.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_fs_surface.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_histogram.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_hpi_fit.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_interpolation.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_inverse_mne.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_inverse_mne_raw.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_inverse_pwl_rap_music.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_inverse_rap_music.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_make_inverse_operator.exe
        REM windeployqtoyqt %BASE_PATH%\out\Release\bin\ex_make_layout.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_read_bem.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_read_epochs.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_read_evoked.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_read_fwd.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_read_fwd_disp_3D.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_read_raw.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_read_write_raw.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_roi_clustered_inverse_pwl_rap_music.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_spectral.exe
        REM windeployqt %BASE_PATH%\out\Release\bin\ex_st_clustered_inverse_pwl_rap_music.exe 

        REM xcopy %BASE_PATH%\src\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll %BASE_PATH%\out\Release\apps\ /i
        
        IF "%PACK_OPTION%"=="pack" (
            cd %BASE_PATH%
            REM Delete folders which we do not want to ship
            rmdir %OUT_DIR_NAME%\resources\data /s /q 
            REM Creating archive of all win deployed applications
            7z a %BASE_PATH%\mne-cpp-windows-dynamic-x86_64.zip %OUT_DIR_NAME%
        )

    ) ELSE IF "%LINK_OPTION%"=="static" (
        
        IF "%PACK_OPTION%"=="pack" (
            REM This script needs to be run from the top level mne-cpp repo folder
            REM Delete folders which we do not want to ship
            rmdir %BASE_PATH%\out\Release\bin\resources /s /q
            rmdir %BASE_PATH%\out\Release\bin\apps\mne_rt_server_plugins /s /q
            rmdir %BASE_PATH%\out\Release\bin\apps\mne_scan_plugins /s /q
            rmdir %BASE_PATH%\out\Release\bin\apps\mne_analyze_plugins /s /q
            
            REM Creating archive of everything in the bin directory
            7z a %BASE_PATH%\mne-cpp-windows-static-x86_64.zip %BASE_PATH%\out\Release        
        )
        
    ) ELSE (
        ECHO Your link option: %LINK_OPTION%
        ECHO Linkage option not defined. 
        ECHO Use: static or dynamic.
    )
        
    :; # ########## WINDOWS SECTION ENDS ####################
    :; # ####################################################
    exit /b
BATCH

if [ "$(uname)" == "Darwin" ]; then
    
    # ######################################################
    # ############## MAC SECTION ###########################

    LINK_OPTION=$1
    PACK_OPTION=$2
    SCRIPT_PATH="$(
        cd "$(dirname "$0")" >/dev/null 2>&1
        pwd -P
    )"
    BASE_PATH=${SCRIPT_PATH}/..

    if [ -z ${LINK_OPTION} ]; then
        LINK_OPTION=dynamic
    fi

    if [[ ${LINK_OPTION} == dynamic ]]; then

        cd ${BASE_PATH}

        # Call macdeployqt on all .app bundles in the bin folder
        for f in ./out/Release/apps/*.app; do $Qt5_DIR/bin/macdeployqt $f ; done

        # Solve for dependencies for mne_scan.app bundle
        cp -a out/Release/apps/mne_scan_plugins/. out/Release/apps/mne_scan.app/Contents/MacOS/mne_scan_plugins
        cp -a out/Release/apps/resources/. out/Release/apps/mne_scan.app/Contents/MacOS/resources
        cp -a src/applications/mne_scan/plugins/brainflowboard/brainflow/installed/out/Release/lib/. out/Release/apps/mne_scan.app/Contents/Frameworks
        cp -a src/applications/mne_scan/plugins/lsladapter/liblsl/build/install/out/Release/lib/. out/Release/apps/mne_scan.app/Contents/Frameworks
        cp -a out/Release/lib/. out/Release/apps/mne_scan.app/Contents/Frameworks
        # cp -a $Qt5_DIR/plugins/renderers/. out/Release/apps/mne_scan.app/Contents/PlugIns/renderers

        # Solve for dependencies for mne_analyze.app bundle
        cp -a out/Release/apps/mne_analyze_plugins/. out/Release/apps/mne_analyze.app/Contents/MacOS/mne_analyze_plugins
        cp -a out/Release/apps/resources/. out/Release/apps/mne_analyze.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_analyze.app/Contents/Frameworks
        # cp -a $Qt5_DIR/plugins/renderers/. out/Release/apps/mne_analyze.app/Contents/PlugIns/renderers

        # Solve for dependencies for mne_rt_server.app bundle
        cp -a out/Release/apps/mne_rt_server_plugins/. out/Release/apps/mne_rt_server.app/Contents/MacOS/mne_rt_server_plugins
        cp -a out/Release/apps/resources/. out/Release/apps/mne_rt_server.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_rt_server.app/Contents/Frameworks

        # Solve for dependencies for mne_forward_solution.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_forward_solution.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_forward_solution.app/Contents/Frameworks

        # Solve for dependencies for mne_dipole_fit.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_dipole_fit.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_dipole_fit.app/Contents/Frameworks

        # Solve for dependencies for mne_anonymize.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_anonymize.app/Contents/MacOS/resources
        cp -a out/Release/lib/. out/Release/apps/mne_anonymize.app/Contents/Frameworks

        if [[ ${PACK_OPTION} == pack ]]; then

            # Delete folders which we do not want to ship
            rm -r out/Release/resouces/data
            # delete these folders because they are in the macos app containers already
            rm -r out/Release/apps/mne_scan_plugins
            rm -r out/Release/apps/mne_analyze_plugins
            rm -r out/Release/apps/mne_rt_server_plugins

            # Creating archive of all macos deployed applications
            tar cfvz mne-cpp-macos-dynamic-x86_64.tar.gz out/Release/apps/.
        fi

    elif [[ ${LINK_OPTION} == static ]]; then

        cd ${BASE_PATH}

        # This script needs to be run from the top level mne-cpp repo folder
        # Solve for dependencies for mne_scan.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_scan.app/Contents/MacOS/resources

        # Solve for dependencies for mne_analyze.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_analyze.app/Contents/MacOS/resources

        # Solve for dependencies for mne_rt_server.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_rt_server.app/Contents/MacOS/resources

        # Solve for dependencies for mne_forward_solution.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_forward_solution.app/Contents/MacOS/resources

        # Solve for dependencies for mne_dipole_fit.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_dipole_target_compile_definitions(mytgt PRIVATE BUILT_SHARED=$<BOOL:${BUILD_SHARED_LIBS}>) # or using `if()` for the bool conversiofit.app/Contents/MacOS/resources

        # Solve for dependencies for mne_anonymize.app bundle
        cp -a out/Release/apps/resources/. out/Release/apps/mne_anonymize.app/Contents/MacOS/resources

        if [[ ${PACK_OPTION} == pack ]]; then
            # Delete folders which we do not want to ship
            cp -r out/Release/ mne-cpp
            rm -r mne-cpp/resources/data
            rm -r mne-cpp/apps/mne_scan_plugins
            rm -r mne-cpp/apps/mne_analyze_plugins
            rm -r mne-cpp/apps/mne_rt_server_plugins

            # Creating archive of all macos deployed applications
            tar cfvz mne-cpp-macos-static-x86_64.tar.gz mne-cpp
        fi

    else 
        echo "Input argument link_option is invalid."
        echo "Input argument link_option is set to ${LINK_OPTION}."
        echo "Use: static or dynamic"
    fi

    # ############## MAC SECTION ENDS ######################
    # ######################################################

elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    
    # ######################################################
    # ############## LINUX SECTION #########################

    LINK_OPTION=$1
    PACK_OPTION=$2
    SCRIPT_PATH="$(
        cd "$(dirname "$0")" >/dev/null 2>&1
        pwd -P
    )"
    BASE_PATH=${SCRIPT_PATH}/../..

    if [ -z ${LINK_OPTION} ]; then
        LINK_OPTION=dynamic
    fi

    if [[ ${LINK_OPTION} == dynamic ]]; then

        # Copy additional brainflow libs
        cp -a ${BASE_PATH}/src/applications/mne_scan/plugins/brainflowboard/brainflow/installed/out/Release/lib/. ${BASE_PATH}/out/Release/lib/

        # Copy additional LSL libs
        cp -a ${BASE_PATH}/src/applications/mne_scan/plugins/lsladapter/liblsl/build/install/out/Release/lib/. ${BASE_PATH}/out/Release/lib/

        # Install some additional packages so linuxdeployqt can find them
        sudo apt-get update
        sudo apt-get install libxkbcommon-x11-0
        sudo apt-get install libxcb-icccm4
        sudo apt-get install libxcb-image0
        sudo apt-get install libxcb-keysyms1
        sudo apt-get install libxcb-render-util0
        sudo apt-get install libbluetooth3
        sudo apt-get install libxcb-xinerama0 
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/out/Release/lib/x86_64-linux-gnu/

        cd ${BASE_PATH}

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        # linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        cd ${BASE_PATH}/mne-cpp
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_analyze -verbose2 -extra-plugins=renderers

        # Manually copy in the libxcb-xinerama library which is needed by plugins/platforms/libxcb.so
        cp /usr/out/Release/lib/x86_64-linux-gnu/libxcb-xinerama.so.0 ${BASE_PATH}/mne-cpp/out/Release/lib/

        if [[ ${PACK_OPTION} == pack ]]; then
            echo 
            echo ldd ./out/Release/apps/mne_scan
            ldd ./out/Release/apps/mne_scan

            echo 
            echo ldd ./plugins/platforms/libqxcb.so
            ldd ./plugins/platforms/libqxcb.so

            # Delete folders which we do not want to ship
            cp -r out/Release/ mne-cpp
            rm -r mne-cpp/resources/data

            # Creating archive of everything in current directory
            tar cfvz ../mne-cpp-linux-dynamic-x86_64.tar.gz mne-cpp   
        fi

    elif [[ ${LINK_OPTION} == static ]]; then

        cd ${BASE_PATH}

        sudo apt-get update
        sudo apt-get install libxkbcommon-x11-0
        sudo apt-get install libxcb-icccm4
        sudo apt-get install libxcb-image0
        sudo apt-get install libxcb-keysyms1
        sudo apt-get install libxcb-render-util0
        sudo apt-get install libbluetooth3
        sudo apt-get install libxcb-xinerama0
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/out/Release/lib/x86_64-linux-gnu/

        # Downloading linuxdeployqt from continious release
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

        # Creating a directory for linuxdeployqt to create results 
        sudo mkdir -p -m777 mne-cpp

        # linuxdeployqt uses mne_scan and mne_analyze binary to resolve dependencies
        cd mne-cpp
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_scan -verbose2 -extra-plugins=renderers
        ../linuxdeployqt-continuous-x86_64.AppImage out/Release/apps/mne_analyze -verbose2 -extra-plugins=renderers

        echo
        echo ldd ./out/Release/apps/mne_scan
        ldd ./out/Release/apps/mne_scan

        # Delete folders which we do not want to ship
        cp -r out/Release mne-cpp
        rm -r mne-cpp/resources/data
        rm -r mne-cpp/apps/mne_rt_server_plugins
        rm -r mne-cpp/apps/mne_scan_plugins
        rm -r mne-cpp/apps/mne_analyze_plugins

        if [[ ${PACK_OPTION} == pack ]]; then
            # Creating archive of everything in the bin directory
            tar cfvz ../mne-cpp-linux-static-x86_64.tar.gz mne-cpp
        fi

    else 
        echo "Input argument link_option is invalid."
        echo "Input argument link_option is set to ${LINK_OPTION}."
        echo "Use: static or dynamic"
    fi

    # ############## LINUX SECTION ENDS ####################
    # ######################################################

fi

exit 0

