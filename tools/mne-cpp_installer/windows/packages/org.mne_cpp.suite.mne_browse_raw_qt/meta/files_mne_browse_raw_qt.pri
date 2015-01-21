#
# win32: copy dll's to mne_browse_raw package data dir
#
DATADIR = $${PWD}/../data/mne_browse_raw_qt
DATADIR ~= s,/,\\,g

FILE = $${MNE_BINARY_DIR}/MNE1Generics.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${DATADIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Utils.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${DATADIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Fs.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${DATADIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Fiff.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${DATADIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Mne.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${DATADIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/mne_browse_raw_qt.exe
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${DATADIR}) $$escape_expand(\\n\\t)
