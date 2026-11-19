//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_shared_library.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqSharedLibrary: dlopen/LoadLibraryW behind one interface.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_shared_library.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// DEFINE LOCAL METHODS
//=============================================================================================================

namespace
{

#if defined(_WIN32)
std::wstring widen(const std::string& text)
{
    const int n = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring out(n > 0 ? static_cast<std::size_t>(n) : 0, L'\0');
    if (n > 0) {
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), n);
        out.pop_back();
    }
    return out;
}

//=============================================================================================================

void* open(const std::string& path, std::string& error)
{
    void* handle = reinterpret_cast<void*>(LoadLibraryW(widen(path).c_str()));
    if (handle == nullptr) {
        error = "error " + std::to_string(GetLastError());
    }
    return handle;
}

void close(void* handle) noexcept
{
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

void* lookup(void* handle, const char* name) noexcept
{
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
}

FILE* openPipe(const char* command)
{
    return _popen(command, "r");
}

void closePipe(FILE* pipe)
{
    _pclose(pipe);
}
#else
void* open(const std::string& path, std::string& error)
{
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        const char* text = dlerror();
        error = text != nullptr ? text : "unknown error";
    }
    return handle;
}

void close(void* handle) noexcept
{
    dlclose(handle);
}

void* lookup(void* handle, const char* name) noexcept
{
    return dlsym(handle, name);
}

FILE* openPipe(const char* command)
{
    return popen(command, "r"); // NOLINT(bugprone-command-processor): fixed command, identifiers checked by the caller
}

void closePipe(FILE* pipe)
{
    pclose(pipe);
}
#endif

/** @return The first output line of @p command, empty on failure. */
std::string firstLine(const std::string& command)
{
    FILE* pipe = openPipe(command.c_str());
    if (pipe == nullptr) {
        return {};
    }
    std::string line;
    char buffer[512];
    if (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        line = buffer;
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
    }
    closePipe(pipe);
    return line;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqSharedLibrary::~AcqSharedLibrary()
{
    if (m_handle != nullptr) {
        close(m_handle);
    }
}

//=============================================================================================================

AcqSharedLibrary::AcqSharedLibrary(AcqSharedLibrary&& other) noexcept
: m_handle(std::exchange(other.m_handle, nullptr))
, m_path(std::move(other.m_path))
{
}

//=============================================================================================================

AcqSharedLibrary& AcqSharedLibrary::operator=(AcqSharedLibrary&& other) noexcept
{
    if (this != &other) {
        if (m_handle != nullptr) {
            close(m_handle);
        }
        m_handle = std::exchange(other.m_handle, nullptr);
        m_path = std::move(other.m_path);
    }
    return *this;
}

//=============================================================================================================

AcqExpected<AcqSharedLibrary> AcqSharedLibrary::load(const std::vector<std::string>& candidates)
{
    std::string tried;
    std::string error;
    for (const std::string& candidate : candidates) {
        if (candidate.empty()) {
            continue;
        }
        void* handle = open(candidate, error);
        if (handle != nullptr) {
            AcqSharedLibrary library;
            library.m_handle = handle;
            library.m_path = candidate;
            return library;
        }
        tried += (tried.empty() ? "" : ", ") + candidate;
    }
    return AcqError(AcqErrorCode::BackendUnavailable,
                    "not found (tried " + (tried.empty() ? std::string("nothing") : tried) + "): " + error);
}

//=============================================================================================================

void* AcqSharedLibrary::symbol(const char* name) const noexcept
{
    return m_handle != nullptr ? lookup(m_handle, name) : nullptr;
}

//=============================================================================================================

const std::string& AcqSharedLibrary::path() const noexcept
{
    return m_path;
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::string mne::detail::acqPythonPackageDir(const std::string& package, const std::string& subdirectory)
{
    const auto identifier = [](const std::string& text) {
        return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isalnum(c) != 0 || c == '_'; });
    };
    if (!identifier(package) || !identifier(subdirectory)) {
        return {}; // nothing but identifiers may reach the shell
    }
    const std::string script = "\"import importlib.util,os;s=importlib.util.find_spec('" + package +
        "');print(os.path.join(os.path.dirname(s.origin),'" + subdirectory + "') if s and s.origin else '')\"";
#if defined(_WIN32)
    const char* const quiet = " 2>NUL";
#else
    const char* const quiet = " 2>/dev/null";
#endif
    for (const char* python : {"python3", "python"}) {
        std::string dir = firstLine(std::string(python) + " -c " + script + quiet);
        if (!dir.empty()) {
            return dir;
        }
    }
    return {};
}

//=============================================================================================================

std::string mne::detail::acqSharedLibraryName(const std::string& base)
{
#if defined(_WIN32)
    return base + ".dll";
#elif defined(__APPLE__)
    return "lib" + base + ".dylib";
#else
    return "lib" + base + ".so";
#endif
}

//=============================================================================================================

std::string mne::detail::acqEnvironment(const char* name)
{
#if defined(_WIN32)
    char* value = nullptr;
    std::size_t size = 0;
    std::string out;
    if (_dupenv_s(&value, &size, name) == 0 && value != nullptr) {
        out = value;
    }
    std::free(value);
    return out;
#else
    const char* value = std::getenv(name); // NOLINT(concurrency-mt-unsafe): read once while loading a backend
    return value != nullptr ? value : "";
#endif
}
