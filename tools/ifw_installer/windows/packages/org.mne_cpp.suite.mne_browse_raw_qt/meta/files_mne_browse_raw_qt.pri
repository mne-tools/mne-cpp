#
# win32: copy dll's to mne_browse_raw package data dir
#
DATADIR = $${PWD}/../data
DATADIR ~= s,/,\\,g

exists($$quote($${DATADIR})) {
    QMAKE_PRE_LINK += $${QMAKE_CLEAN} $$quote($${DATADIR}) $$escape_expand(\\n\\t)
}
QMAKE_PRE_LINK += $${QMAKE_MKDIR} $$quote($${DATADIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/mne_browse_raw_qt.exe
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${DATADIR}) $$escape_expand(\\n\\t)
