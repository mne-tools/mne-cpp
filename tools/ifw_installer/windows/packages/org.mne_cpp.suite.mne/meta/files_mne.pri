#
# win32: copy dll's to mne_browse_raw package data dir
#

LIBDIR = $${PWD}/../data
LIBDIR ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_MKDIR} $$quote($${LIBDIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/*.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${LIBDIR}) $$escape_expand(\\n\\t)



#Directory list to copy
DIRS =  bearer \
        iconengines \
        imageformats \
        platforms \
        translations


for(DIR, DIRS) {
    SRCPATH = $${MNE_BINARY_DIR}/$${DIR}
    SRCPATH ~= s,/,\\,g

    DESTPATH = $${LIBDIR}/$${DIR}
    DESTPATH ~= s,/,\\,g

    QMAKE_PRE_LINK += $${QMAKE_MKDIR} $$quote($${DESTPATH}) $$escape_expand(\\n\\t)
    QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${SRCPATH}) $$quote($${DESTPATH}) $$escape_expand(\\n\\t)
}

