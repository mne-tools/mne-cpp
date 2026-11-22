# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Receive a BrainFlow multicast stream with STREAMING_BOARD and write it as float64.

Usage: brainflow_receive.py ADDRESS PORT MASTER_BOARD SECONDS OUT

OUT holds the number of rows and samples as two int64 values, then the rows x samples
array in row-major float64. Exit code 2 when BrainFlow is not installed.
"""

import sys
import time

try:
    import numpy as np
    from brainflow.board_shim import BoardIds, BoardShim, BrainFlowInputParams
except ImportError as error:
    print(f"brainflow not installed: {error}")
    sys.exit(2)

address, port, master, seconds, out = sys.argv[1:6]
BoardShim.disable_board_logger()
params = BrainFlowInputParams()
params.ip_address = address
params.ip_port = int(port)
params.master_board = int(master)
board = BoardShim(BoardIds.STREAMING_BOARD.value, params)
board.prepare_session()
board.start_stream()
time.sleep(float(seconds))
data = board.get_board_data()
board.stop_stream()
board.release_session()
with open(out, "wb") as file:
    np.array(data.shape, dtype=np.int64).tofile(file)
    np.ascontiguousarray(data, dtype=np.float64).tofile(file)
print("OK")
