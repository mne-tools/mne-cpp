#
# win32: copy dll's to mne_browse_raw package data dir
#

LIBDIR = $${PWD}/../data
LIBDIR ~= s,/,\\,g

FILE = $${MNE_BINARY_DIR}/*.dll
FILE ~= s,/,\\,g
QMAKE_PRE_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${LIBDIR}) $$escape_expand(\\n\\t)
