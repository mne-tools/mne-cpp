//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_shared_library.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Runtime loading of third-party device libraries (liblsl, BrainFlow, vendor SDKs) without linking them.
 */

#ifndef ACQ_SHARED_LIBRARY_H
#define ACQ_SHARED_LIBRARY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_global.h"
#include "../acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * A loaded shared library; unloads it on destruction. Move-only.
 */
class ACQSHARED_EXPORT AcqSharedLibrary
{
public:
    AcqSharedLibrary() = default;
    ~AcqSharedLibrary();
    AcqSharedLibrary(AcqSharedLibrary&& other) noexcept;
    AcqSharedLibrary& operator=(AcqSharedLibrary&& other) noexcept;
    AcqSharedLibrary(const AcqSharedLibrary&) = delete;
    AcqSharedLibrary& operator=(const AcqSharedLibrary&) = delete;

    /**
     * Loads the first of @p candidates that loads. A candidate with a directory separator is a path, others
     * are file names looked up by the system loader.
     *
     * @return The library, or BackendUnavailable naming every candidate tried and the last loader error.
     */
    [[nodiscard]] static AcqExpected<AcqSharedLibrary> load(const std::vector<std::string>& candidates);

    /** @return The address of @p name, or nullptr. */
    [[nodiscard]] void* symbol(const char* name) const noexcept;

    /** @return The candidate that was loaded. */
    [[nodiscard]] const std::string& path() const noexcept;

private:
    void* m_handle = nullptr;
    std::string m_path;
};
ACQ_STD_MEMBERS_END

/**
 * @return The library directory of a Python package, from python3 (then python) on the PATH: e.g. "pylsl" and
 *         "lib" give .../site-packages/pylsl/lib. Empty if Python or the package is missing.
 */
ACQSHARED_EXPORT std::string acqPythonPackageDir(const std::string& package, const std::string& subdirectory);

/** @return The platform file name of library @p base: liblsl.dylib, liblsl.so, lsl.dll for "lsl". */
ACQSHARED_EXPORT std::string acqSharedLibraryName(const std::string& base);

/** @return The value of environment variable @p name, empty if it is not set. */
ACQSHARED_EXPORT std::string acqEnvironment(const char* name);

} // namespace mne::detail

#endif // ACQ_SHARED_LIBRARY_H
