!isEmpty(MNE-CPP_INSTALLER_PRI_INCLUDED) {
    error("mne-cpp_installer.pri already included")
}

MNE-CPP_INSTALLER_PRI_INCLUDED = 1

#Adapt to IFW install path
IFW_INSTALL_PATH = D:/GitHub/build-installerfw-Qt_5_3_2-Release
