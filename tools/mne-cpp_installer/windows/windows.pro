TEMPLATE = aux

include (../mne-cpp_installer.pri)

OTHER_FILES = \
    readme.txt \
    config/config.xml \
    packages/org.mne_cpp.suite/meta/installscript.js \
    packages/org.mne_cpp.suite/meta/package.xml \
    packages/org.mne_cpp.suite.mne_browse_raw_qt/meta/package.xml \
    packages/org.mne_cpp.suite.mne_browse_raw_qt/meta/license_mne_browse_raw_qt.txt \
    packages/org.mne_cpp.suite.mne_browse_raw_qt/meta/installscript.qs

FORMS += \
    packages/org.mne_cpp.suite/meta/licensewidget.ui \
    packages/org.mne_cpp.suite/meta/readytoinstallwidget.ui \
    packages/org.mne_cpp.suite/meta/targetwidget.ui

#Offline installer configuration
mne-cpp_installer.target = build_mne-cpp_installer
mne-cpp_installer.commands = $$IFW_INSTALL_PATH/bin/binarycreator --offline-only -c $$PWD/config/config.xml -p $$PWD/packages mne-cpp-windows-x86_64_1.0.0
QMAKE_EXTRA_TARGETS += mne-cpp_installer

default_target.target = first
default_target.depends = mne-cpp_installer
QMAKE_EXTRA_TARGETS += default_target

