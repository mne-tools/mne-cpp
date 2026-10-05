#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the tiny ONNX classifier used by ex_ml and print its NumPy oracle.

Needs ``pip install onnx``. The model is ``softmax(x @ W + b)`` for a batch of
3-feature rows and 2 classes (opset 18, IR 10, readable by ONNX Runtime 1.21),
with the class labels stored in the ``classes`` metadata entry.
"""

from pathlib import Path

import numpy as np
import onnx
from onnx import TensorProto, helper, numpy_helper

W = np.array([[1.0, -1.0], [0.5, 0.25], [-2.0, 2.0]], dtype=np.float32)
b = np.array([0.1, -0.1], dtype=np.float32)

graph = helper.make_graph(
    [
        helper.make_node("Gemm", ["x", "W", "b"], ["logits"]),
        helper.make_node("Softmax", ["logits"], ["probabilities"], axis=1),
    ],
    "affine_softmax",
    [helper.make_tensor_value_info("x", TensorProto.FLOAT, ["batch", 3])],
    [helper.make_tensor_value_info("probabilities", TensorProto.FLOAT, ["batch", 2])],
    initializer=[numpy_helper.from_array(W, "W"), numpy_helper.from_array(b, "b")],
)
model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 18)])
model.ir_version = 10
helper.set_model_props(model, {"classes": "rest,task"})
onnx.checker.check_model(model)
onnx.save(model, Path(__file__).parent / "affine_softmax.onnx")

x = np.array([[1.0, 2.0, 0.5], [0.0, -1.0, 1.5]], dtype=np.float32)
logits = x @ W + b
p = np.exp(logits - logits.max(axis=1, keepdims=True))
print(p / p.sum(axis=1, keepdims=True))
