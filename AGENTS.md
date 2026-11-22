# Agent Instructions

Guidance for coding agents working in this repository. The coding conventions are documented in
[contr-style.mdx](doc/website/docs/development/contr-style.mdx); the rules below are the ones enforced by tooling.

## Formatting

- C/C++ under `src/` (excluding `src/3rdparty`) is formatted with the repository [.clang-format](.clang-format),
  using the pinned clang-format version (`pip install clang-format==21.1.8`).
- Check or fix changed files before committing:
  ```bash
  python3 scripts/quality/check_format.py          # files changed vs origin/staging
  python3 scripts/quality/check_format.py --fix    # reformat them in place
  ```
- Do not fight the formatter by hand and avoid formatting-only changes to unrelated code.

## Include sections

Includes are grouped under banners in this order; a section without includes has no banner
(checked and fixed by `check_format.py`, see [include_sections.py](scripts/quality/include_sections.py)):

```cpp
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "own_header.h"
#include <mne/base/other_mne_cpp_header.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================
```

Keep `//====...` separator lines unchanged; do not wrap or shorten them.

## File header

New `.h` and `.cpp` files start with the SPDX header:

```cpp
//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) <year> MNE-CPP Authors
 *
 * @file     <filename>
 * @author   <name> <email>
 * @since    <version>
 * @date     <Month Year>
 * @brief    <Short description of the file contents>.
 */
```

Include guards follow `<MODULE>_<FILE>_H`, e.g. `BASE_XML_H`.

## Naming

Classes `MyClass`, member functions `myFunction`, member variables `m_typeMeaningfulName`. Document new APIs with
Doxygen comments including `@since`.

## Commits

Prefix commit messages and PR titles with `FIX:`, `ENH:`, `MAINT:`, `DOC:`, `TEST:` or `CI:`.
