# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Exported classes whose members hold only forward-declared types.

MSVC defines the implicit destructor of a dllexport class in every file of the
library that sees the class, so a std::vector or std::optional member of a type
that the header only forward-declares fails there, while GCC and Clang compile
it. Such a header includes the type or declares the destructor.

@since 3.0.0
"""

import re
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]

_FORWARD = re.compile(r"(?:^|[{;])\s*(?:class|struct)\s+(\w+)\s*;", re.MULTILINE)
_EXPORTED = re.compile(r"^\s*(?:class|struct)\s+\w*EXPORT\w*\s+(\w+)\b[^;]*?\{", re.MULTILINE)
_MEMBER = re.compile(r"std::(?:vector|deque|list|optional)\s*<\s*(?:\w+::)*(\w+)\s*>\s+\w+\s*(?:;|=|\{)")


def snake(name: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z][a-z])", "_", name).lower()


def class_body(text: str, start: int) -> str:
    """The text from just after a class's opening brace to its closing brace."""
    depth = 1
    for position in range(start, len(text)):
        depth += {"{": 1, "}": -1}.get(text[position], 0)
        if depth == 0:
            return text[start:position]
    return text[start:]


def incomplete_members(roots: list[Path], base: Path = REPO_ROOT) -> list[str]:
    """Lists '<header>: <class> holds <type>' for every exported class with such a member and no declared destructor."""
    found = []
    for root in roots:
        for header in sorted(root.rglob("*.h")):
            if "3rdparty" in header.parts:
                continue
            text = header.read_text(encoding="utf-8", errors="replace")
            forward = set(_FORWARD.findall(text))
            if not forward:
                continue
            included = {Path(m).stem.replace("_", "") for m in re.findall(r'#include\s*[<"]([^>"]+)[>"]', text)}
            for match in _EXPORTED.finditer(text):
                body = class_body(text, match.end())
                if re.search(rf"~\s*{match.group(1)}\s*\(", body):
                    continue
                for held in sorted(set(_MEMBER.findall(body)) & forward):
                    if snake(held).replace("_", "") not in included:
                        found.append(f"{header.relative_to(base).as_posix()}: {match.group(1)} holds {held}")
    return found


class TestIncompleteMembers(unittest.TestCase):
    def test_exported_classes_hold_complete_types(self):
        self.assertEqual(incomplete_members([REPO_ROOT / "src"]), [])

    def test_detects_a_forward_declared_member(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "worker.h").write_text(
                "namespace mne { class FsLabel; }\n"
                "class DISP3DSHARED_EXPORT Worker : public QObject\n{\n    std::vector<mne::FsLabel> m_labels;\n};\n")
            self.assertEqual(incomplete_members([root], root), ["worker.h: Worker holds FsLabel"])

    def test_accepts_a_declared_destructor_or_an_include(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a.h").write_text("class FsLabel;\nclass X_EXPORT A\n{\n    ~A();\n    std::vector<FsLabel> m_labels;\n};\n")
            (root / "b.h").write_text('#include <mne/fs/fs_label.h>\nclass FsLabel;\nclass X_EXPORT B\n{\n    std::vector<FsLabel> m_labels;\n};\n')
            self.assertEqual(incomplete_members([root], root), [])


if __name__ == "__main__":
    unittest.main()
