TEMPLATE = subdirs

include (../../mne-cpp.pri)
include (mne-cpp_installer.pri)

win32 {
    SUBDIRS += \
        windows
}

unix:!macx {
    SUBDIRS += \
        linux
}

macx {
    SUBDIRS += \
        mac
}

RESOURCES += \
