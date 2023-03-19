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
    
    SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\"

    IF "%LINK_OPTION%"=="" (
        SET LINK_OPTION=dynamic
    )

    IF "%LINK_OPTION%"=="dynamic" (
        
        Rem Solve dependencies for libraries
        windeployqt %BASE_PATH%\out\Release\mne_utils.dll
        windeployqt %BASE_PATH%\out\Release\mne_fiff.dll
        windeployqt %BASE_PATH%\out\Release\mne_fs.dll
        windeployqt %BASE_PATH%\out\Release\mne_events.dll
        windeployqt %BASE_PATH%\out\Release\mne_mne.dll
        windeployqt %BASE_PATH%\out\Release\mne_fwd.dll
        windeployqt %BASE_PATH%\out\Release\mne_inverse.dll
        windeployqt %BASE_PATH%\out\Release\mne_communication.dll
        windeployqt %BASE_PATH%\out\Release\mne_rtprocessing.dll
        windeployqt %BASE_PATH%\out\Release\mne_connectivity.dll
        windeployqt %BASE_PATH%\out\Release\mne_disp.dll
        windeployqt %BASE_PATH%\out\Release\mne_disp3D.dll

        REM solve dependencies for applications
        windeployqt %BASE_PATH%\out\Release\mne_analyze
        windeployqt %BASE_PATH%\out\Release\mne_scan
        windeployqt %BASE_PATH%\out\Release\mne_anonymize
        windeployqt %BASE_PATH%\out\Release\mne_rt_server
        windeployqt %BASE_PATH%\out\Release\mne_forward_solution
        windeployqt %BASE_PATH%\out\Release\mne_edf2fiff
        windeployqt %BASE_PATH%\out\Release\mne_dipole_fit

        REM solve dependencies for tests 
        windeployqt %BASE_PATH%\out\Release\test_coregistration.exe
        windeployqt %BASE_PATH%\out\Release\test_dipole_fit.exe
        windeployqt %BASE_PATH%\out\Release\test_edf2fiff_rwr.exe
        windeployqt %BASE_PATH%\out\Release\test_fiff_coord_trans.exe
        windeployqt %BASE_PATH%\out\Release\test_fiff_cov.exe
        windeployqt %BASE_PATH%\out\Release\test_fiff_digitizer.exe
        windeployqt %BASE_PATH%\out\Release\test_fiff_mne_types_io.exe
        windeployqt %BASE_PATH%\out\Release\test_fiff_rwr.exe
        windeployqt %BASE_PATH%\out\Release\test_filtering.exe
        windeployqt %BASE_PATH%\out\Release\test_geometryinfo.exe
        windeployqt %BASE_PATH%\out\Release\test_hpiDataUpdater.exe
        windeployqt %BASE_PATH%\out\Release\test_hpiFit.exe
        windeployqt %BASE_PATH%\out\Release\test_hpiFit_integration.exe
        windeployqt %BASE_PATH%\out\Release\test_hpiModelParameter.exe
        windeployqt %BASE_PATH%\out\Release\test_interpolation.exe
        windeployqt %BASE_PATH%\out\Release\test_mne_anonymize.exe
        windeployqt %BASE_PATH%\out\Release\test_mne_forward_solution.exe
        windeployqt %BASE_PATH%\out\Release\test_mne_msh_display_surface_set.exe
        windeployqt %BASE_PATH%\out\Release\test_mne_project_to_surface.ex
        windeployqt %BASE_PATH%\out\Release\test_sensorSet.exe
        windeployqt %BASE_PATH%\out\Release\test_signalModel.exe
        windeployqt %BASE_PATH%\out\Release\test_spectral_connectivity.exe
        windeployqt %BASE_PATH%\out\Release\test_utils_circularbuffer.exe

        Rem solve dependencies with test
        windeployqt %BASE_PATH%\out\Release\ex_averaging.exe
        windeployqt %BASE_PATH%\out\Release\ex_cancel_noise.exe
        windeployqt %BASE_PATH%\out\Release\ex_clustered_inverse_mne.exe
        windeployqt %BASE_PATH%\out\Release\ex_clustered_inverse_mne_raw.exe
        windeployqt %BASE_PATH%\out\Release\ex_clustered_inverse_pwl_rap_music_raw.exe
        windeployqt %BASE_PATH%\out\Release\ex_clustered_inverse_rap_music_raw.exe    
        windeployqt %BASE_PATH%\out\Release\ex_compute_forward.pro.exe
        windeployqt %BASE_PATH%\out\Release\ex_connectivity.exe
        windeployqt %BASE_PATH%\out\Release\ex_connectivity_comparison.exe
        windeployqt %BASE_PATH%\out\Release\ex_connectivity_performace.exe
        windeployqt %BASE_PATH%\out\Release\ex_coreg.exe
        windeployqt %BASE_PATH%\out\Release\ex_disp.exe
        windeployqt %BASE_PATH%\out\Release\ex_disp3D.exe
        windeployqt %BASE_PATH%\out\Release\ex_evoked_grad_amp.exe
        windeployqt %BASE_PATH%\out\Release\ex_fiff.exe
        windeployqt %BASE_PATH%\out\Release\ex_fiff_sniffer.exe
        windeployqt %BASE_PATH%\out\Release\ex_file_utils.exe
        windeployqt %BASE_PATH%\out\Release\ex_filtering.exe
        windeployqt %BASE_PATH%\out\Release\ex_find_Evoked.exe
        windeployqt %BASE_PATH%\out\Release\ex_fs_surface.exe
        windeployqt %BASE_PATH%\out\Release\ex_histogram.exe
        windeployqt %BASE_PATH%\out\Release\ex_hpi_fit.exe
        windeployqt %BASE_PATH%\out\Release\ex_interpolation.exe
        windeployqt %BASE_PATH%\out\Release\ex_inverse_mne.exe
        windeployqt %BASE_PATH%\out\Release\ex_inverse_mne_raw.exe
        windeployqt %BASE_PATH%\out\Release\ex_inverse_pwl_rap_music.exe
        windeployqt %BASE_PATH%\out\Release\ex_inverse_rap_music.exe
        windeployqt %BASE_PATH%\out\Release\ex_make_inverse_operator.exe
        windeployqt %BASE_PATH%\out\Release\ex_make_layout.exe
        windeployqt %BASE_PATH%\out\Release\ex_read_bem.exe
        windeployqt %BASE_PATH%\out\Release\ex_read_epochs.exe
        windeployqt %BASE_PATH%\out\Release\ex_read_evoked.exe
        windeployqt %BASE_PATH%\out\Release\ex_read_fwd.exe
        windeployqt %BASE_PATH%\out\Release\ex_read_fwd_disp_3D.exe
        windeployqt %BASE_PATH%\out\Release\ex_read_raw.exe
        windeployqt %BASE_PATH%\out\Release\ex_read_write_raw.exe
        windeployqt %BASE_PATH%\out\Release\ex_roi_clustered_inverse_pwl_rap_music.exe
        windeployqt %BASE_PATH%\out\Release\ex_spectral.exe
        windeployqt %BASE_PATH%\out\Release\ex_st_clustered_inverse_pwl_rap_music.exe 

        Rem xcopy %BASE_PATH%\src\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll %BASE_PATH%\out\Release\apps\ /i
        
        IF "%PACK_OPTION%"=="pack" (
            Rem Delete folders which we do not want to ship
            rmdir %BASE_PATH%\out\Release\resources\data /s /q 
            Rem Creating archive of all win deployed applications
            7z a %BASE_PATH%\mne-cpp-windows-dynamic-x86_64.zip %BASE_PATH%\out\Release
        )

    ) ELSE IF "%LINK_OPTION%"=="static" (
        
        IF "%PACK_OPTION%"=="pack" (
            Rem This script needs to be run from the top level mne-cpp repo folder
            Rem Delete folders which we do not want to ship
            rmdir %BASE_PATH%\out\Release\resources /s /q
            rmdir %BASE_PATH%\out\Release\apps\mne_rt_server_plugins /s /q
            rmdir %BASE_PATH%\out\Release\apps\mne_scan_plugins /s /q
            rmdir %BASE_PATH%\out\Release\apps\mne_analyze_plugins /s /q
            
            Rem Creating archive of everything in the bin directory
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

