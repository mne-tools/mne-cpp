# Formatting & Code Style Rules (C++ / Qt / MNE-CPP)

These rules define the canonical formatting for all C++ code in this repository.
Agents must follow them strictly.

---

## Source of Truth

- Use **clang-format** for all C++ formatting.
- The canonical configuration is the repository root `.clang-format` file.
- Do not invent or drift from the configured style.

---

## How to Format

- Before submitting changes that touch C++ code, run clang-format on modified files.
- Prefer formatting only changed regions when possible to avoid noisy diffs.
- Recommended commands:
  - `clang-format -i <file>`
  - `git clang-format`

---

## Non-Negotiable Style Decisions

(as enforced by `.clang-format`)

- Indentation: **4 spaces**, no tabs.
- Braces: attached (`class Foo {`).
- Namespace contents are not extra-indented.
- Access specifiers (`public:` etc.) aligned with class indentation.
- No alignment of consecutive assignments or declarations.
- Constructor initializer lists:
  - Colon on its own line.
  - One initializer per line.
  - No continuation indentation.
- Do **not** auto-sort includes.
- Do not reflow comments or license blocks.

---

## Known Formatter Limitations

- clang-format cannot preserve a space before empty function parentheses:

Foo ()


Expect normalization to:

Foo()


- Do not manually fight the formatter unless explicitly instructed.

---

## Include Handling

- Preserve include section headers:

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


- Maintain manual grouping.
- Keep ordering stable unless functionally required.

---

## Comment & Banner Rules

- Separator lines must remain unchanged:

//=============================================================================================================


- Do not wrap or shorten.
- Do not reflow Doxygen or license blocks.

---

# File Header Template (MANDATORY)

All new `.h` and `.cpp` files MUST begin with this exact structure.

---

## Canonical Header Pattern

```cpp
//=============================================================================================================
/**
* @file     <filename>
* @author   <author name> <email>
* @since    <version>
* @date     <Month, Year>
*
* @section  LICENSE
*
* Copyright (C) <year>, <author(s)>. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    <Short description of the file contents>.
*
*/
Header Rules
The separator must be exactly:

//=============================================================================================================
Replace placeholders only — never change structure.

Do not compress or rewrite license text.

This header comes before includes, namespaces, or pragmas.

Canonical Examples
These examples show the expected output shape after formatting.

Header Layout
#ifndef MAINSPLASHSCREEN_H
#define MAINSPLASHSCREEN_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSplashScreen>
#include <QSharedPointer>

namespace MNESCAN
{

class MainSplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    typedef QSharedPointer<MainSplashScreen> SPtr;
    typedef QSharedPointer<const MainSplashScreen> ConstSPtr;

    MainSplashScreen();
    explicit MainSplashScreen(const QPixmap & pixmap);
    MainSplashScreen(const QPixmap & pixmap, Qt::WindowFlags f);

    virtual ~MainSplashScreen();
};

} // NAMESPACE

#endif // MAINSPLASHSCREEN_H
Constructor Definitions
MainSplashScreen::MainSplashScreen()
: MainSplashScreen(QPixmap())
{
}

MainSplashScreen::MainSplashScreen(const QPixmap & pixmap)
: MainSplashScreen(pixmap, Qt::Widget)
{
}

MainSplashScreen::MainSplashScreen(const QPixmap & pixmap, Qt::WindowFlags f)
: QSplashScreen(pixmap, f)
{
}
Destructor
MainSplashScreen::~MainSplashScreen()
{
    // ToDo cleanup work
}
Enforcement Philosophy
Prefer mechanical formatting over manual tweaks.

Avoid formatting-only commits unless required.

When touching legacy code, reformat only the modified regions.

If output deviates from these examples, treat it as an error.


---

If you want, the next logical step is a **`.agent/rules/file-structure.md`** covering:

- include guard naming,
- header/implementation pairing,
- namespace rules,
- directory layout,
- Qt macro placement,

which would make agents nearly indistinguishable from a human contributor in this repo.