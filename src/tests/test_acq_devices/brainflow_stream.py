# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Stream BrainFlow's synthetic board with add_streamer and write what BrainFlow itself read.

Usage: brainflow_stream.py ADDRESS PORT SECONDS OUT [FILE]

The board streams to streaming_board://ADDRESS:PORT, and to FILE with BrainFlow's file streamer if given, and
inserts marker 7 after a second. OUT holds the number of rows
and samples as two int64 values, then the rows x samples array in row-major float64. Exit code 2 when BrainFlow is
not installed.
"""

import sys
import time

try:
    import numpy as np
    from brainflow.board_shim import BoardIds, BoardShim, BrainFlowInputParams
except ImportError as error:
    print(f"brainflow not installed: {error}")
    sys.exit(2)

address, port, seconds, out = sys.argv[1:5]
BoardShim.disable_board_logger()
board = BoardShim(BoardIds.SYNTHETIC_BOARD.value, BrainFlowInputParams())
board.prepare_session()
board.add_streamer(f"streaming_board://{address}:{port}")
if len(sys.argv) > 5:
    board.add_streamer(f"file://{sys.argv[5]}:w")
board.start_stream()
time.sleep(1.0)
board.insert_marker(7)
time.sleep(float(seconds) - 1.0)
board.stop_stream()
data = board.get_board_data()
board.release_session()
with open(out, "wb") as file:
    np.array(data.shape, dtype=np.int64).tofile(file)
    np.ascontiguousarray(data, dtype=np.float64).tofile(file)
print("OK")
