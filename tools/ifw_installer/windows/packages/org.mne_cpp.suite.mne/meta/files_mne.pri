#
# win32: copy dll's to mne_browse_raw package data dir
#

LIBDIR = $${PWD}/../data/MNE/libs
LIBDIR ~= s,/,\\,g

FILE = $${MNE_BINARY_DIR}/MNE1Generics.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${LIBDIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Utils.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${LIBDIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Fs.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${LIBDIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Fiff.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${LIBDIR}) $$escape_expand(\\n\\t)

FILE = $${MNE_BINARY_DIR}/MNE1Mne.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${LIBDIR}) $$escape_expand(\\n\\t)
