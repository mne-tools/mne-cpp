TEMPLATE = subdirs

include (../../mne-cpp.pri)

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
