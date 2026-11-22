//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     python_guard.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    QtTest guards that skip or fail a test slot when Python or a Python package is missing.
 *
 * @c GUARD_PYTHON / @c GUARD_PYTHON_PACKAGE @c QSKIP a test cleanly when Python is missing on developer machines but
 * escalate to @c QFAIL when @c MNE_REQUIRE_PYTHON=true, so CI runs can never silently skip cross-language regression
 * coverage. They lived in @c pybridge/python_test_helper.h until 3.0.0; they expand to QtTest macros, so they belong
 * to the QtTest test frames and keep @c mne_pybridge free of Qt.
 */

#ifndef MNE_TESTFRAMES_PYTHON_GUARD_H
#define MNE_TESTFRAMES_PYTHON_GUARD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/pybridge/python_test_helper.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// DEFINE MACROS
//=============================================================================================================

/**
 * @brief Skips (or fails) the current QtTest slot if a condition does not hold.
 *
 * Without @c MNE_REQUIRE_PYTHON the slot is skipped with a clear message; with @c MNE_REQUIRE_PYTHON=true it fails,
 * so a missing Python installation is never silently ignored in CI.
 *
 * @param available  Condition, e.g. @c helper.is_available().
 * @param reason     Message, anything a @c std::string can be constructed from.
 */
#define GUARD_PYTHON(available, reason)                                                  \
    do {                                                                                 \
        if (!(available)) {                                                              \
            const std::string guardReason_(reason);                                      \
            if (mne::PythonTestHelper::is_python_required())                             \
                QFAIL(("HARD FAIL (MNE_REQUIRE_PYTHON=true): " + guardReason_).c_str()); \
            QSKIP(("SKIP: " + guardReason_).c_str());                                    \
        }                                                                                \
    } while (0)

/**
 * @brief Skips (or fails) the current QtTest slot if a Python package is missing.
 *
 * @param helper       A @c mne::PythonTestHelper.
 * @param packageName  Module name, e.g. "sklearn".
 */
#define GUARD_PYTHON_PACKAGE(helper, packageName) \
    GUARD_PYTHON((helper).has_package(packageName), std::string("Python package '") + (packageName) + "' not available")

#endif // MNE_TESTFRAMES_PYTHON_GUARD_H
