//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     process.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    run_process() with posix_spawn or CreateProcess, and the host description.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "process.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <system_error>

#if defined(_WIN32)
#include <cwchar>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#elif !defined(__EMSCRIPTEN__)
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <poll.h>
#include <spawn.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <net/if_dl.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <netpacket/packet.h>
#include <sys/socket.h>
#endif
extern char** environ; // NOLINT(readability-redundant-declaration): not declared by every libc header
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

constexpr std::size_t kChunk = 65536;
constexpr std::chrono::milliseconds kPoll{20};

/** Hands output to the callback or appends it to the result. */
void deliver(const ProcessOptions& options, ProcessResult& result, ProcessChannel channel, std::string_view data)
{
    if (data.empty())
        return;
    if (options.on_output)
        options.on_output(channel, data);
    else
        (channel == ProcessChannel::StdOut ? result.std_out : result.std_err).append(data);
}

#if defined(_WIN32)

std::wstring widen(std::string_view utf8)
{
    if (utf8.empty())
        return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), n);
    return wide;
}

/** Quotes one argument so that the C runtime's command-line parser gives it back unchanged. */
void append_quoted(std::wstring& line, const std::wstring& arg)
{
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        line += arg;
        return;
    }
    line += L'"';
    std::size_t backslashes = 0;
    for (const wchar_t c : arg) {
        if (c == L'\\') {
            ++backslashes;
            continue;
        }
        line.append(c == L'"' ? (2 * backslashes) + 1 : backslashes, L'\\');
        backslashes = 0;
        line += c;
    }
    line.append(2 * backslashes, L'\\');
    line += L'"';
}

/** The caller's environment with @p overrides applied, as a CreateProcess block; empty when nothing changes. */
std::wstring environment_block(const std::vector<std::pair<std::string, std::string>>& overrides)
{
    if (overrides.empty())
        return {};
    std::vector<std::wstring> entries;
    if (wchar_t* strings = GetEnvironmentStringsW()) {
        for (const wchar_t* p = strings; *p != L'\0'; p += std::wcslen(p) + 1)
            entries.emplace_back(p);
        FreeEnvironmentStringsW(strings);
    }
    for (const auto& [key, value] : overrides) {
        const std::wstring wkey = widen(key);
        std::erase_if(entries, [&](const std::wstring& entry) {
            return entry.size() > wkey.size() && entry[wkey.size()] == L'=' &&
                CompareStringOrdinal(entry.data(), static_cast<int>(wkey.size()), wkey.data(), static_cast<int>(wkey.size()), TRUE) == CSTR_EQUAL;
        });
        entries.push_back(wkey + L'=' + widen(value));
    }
    std::wstring block;
    for (const std::wstring& entry : entries) {
        block += entry;
        block += L'\0';
    }
    block += L'\0';
    return block;
}

/** Reads whatever @p pipe holds without blocking; closes it at end of file. */
void drain(HANDLE& pipe, ProcessChannel channel, const ProcessOptions& options, ProcessResult& result, std::array<char, kChunk>& buffer)
{
    while (pipe != nullptr) {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) {
            CloseHandle(pipe);
            pipe = nullptr;
            return;
        }
        if (available == 0)
            return;
        DWORD read = 0;
        if (!ReadFile(pipe, buffer.data(), static_cast<DWORD>(std::min<std::size_t>(available, buffer.size())), &read, nullptr) || read == 0)
            return;
        deliver(options, result, channel, std::string_view(buffer.data(), read));
    }
}

#elif !defined(__EMSCRIPTEN__)

bool make_pipe(std::array<int, 2>& fds)
{
    if (pipe(fds.data()) != 0)
        return false;
    for (const int fd : fds)
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    return true;
}

/** Makes the child start in @p dir. */
void add_chdir(posix_spawn_file_actions_t& actions, const char* dir)
{
#if defined(__APPLE__)
    if (__builtin_available(macOS 26.0, *)) {
        posix_spawn_file_actions_addchdir(&actions, dir);
        return;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    posix_spawn_file_actions_addchdir_np(&actions, dir);
#pragma clang diagnostic pop
#else
    posix_spawn_file_actions_addchdir_np(&actions, dir);
#endif
}

/** Reads whatever @p fd holds; closes it at end of file. */
void drain(int& fd, ProcessChannel channel, const ProcessOptions& options, ProcessResult& result, std::array<char, kChunk>& buffer)
{
    const ssize_t n = read(fd, buffer.data(), buffer.size());
    if (n > 0) {
        deliver(options, result, channel, std::string_view(buffer.data(), static_cast<std::size_t>(n)));
    } else if (n == 0 || errno != EINTR) {
        close(fd);
        fd = -1;
    }
}

#endif

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

ProcessResult mne::run_process(const std::filesystem::path& program, std::span<const std::string> args, const ProcessOptions& options)
{
    ProcessResult result;
    using Clock = std::chrono::steady_clock;
    const auto deadline = options.timeout ? std::optional(Clock::now() + *options.timeout) : std::nullopt;
    std::array<char, kChunk> buffer{};

#if defined(__EMSCRIPTEN__)
    static_cast<void>(program);
    static_cast<void>(args);
    static_cast<void>(deadline);
    static_cast<void>(buffer);
    result.error = "processes are not available in WebAssembly";
    return result;

#elif defined(_WIN32)
    std::wstring line;
    append_quoted(line, program.wstring());
    for (const std::string& arg : args) {
        line += L' ';
        append_quoted(line, widen(arg));
    }
    std::wstring environment = environment_block(options.environment);

    SECURITY_ATTRIBUTES inherit{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE outRead = nullptr;
    HANDLE outWrite = nullptr;
    HANDLE errRead = nullptr;
    HANDLE errWrite = nullptr;
    HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &inherit, OPEN_EXISTING, 0, nullptr);
    if (!CreatePipe(&outRead, &outWrite, &inherit, 0) || !CreatePipe(&errRead, &errWrite, &inherit, 0)) {
        result.error = std::system_category().message(static_cast<int>(GetLastError()));
        for (HANDLE h : {input, outRead, outWrite, errRead, errWrite}) {
            if (h != nullptr && h != INVALID_HANDLE_VALUE)
                CloseHandle(h);
        }
        return result;
    }
    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);

    // Only these three handles reach the child, so concurrent runs cannot keep each other's pipes open
    std::array<HANDLE, 3> inherited{input, outWrite, errWrite};
    SIZE_T attributeSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
    std::vector<std::byte> attributeStorage(attributeSize);
    auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
    InitializeProcThreadAttributeList(attributes, 1, 0, &attributeSize);
    UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited.data(), sizeof(inherited), nullptr, nullptr);

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input;
    startup.StartupInfo.hStdOutput = outWrite;
    startup.StartupInfo.hStdError = errWrite;
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION info{};
    const std::wstring workingDir = options.working_dir.wstring();
    const BOOL created = CreateProcessW(nullptr, line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT,
                                        environment.empty() ? nullptr : environment.data(), workingDir.empty() ? nullptr : workingDir.c_str(),
                                        &startup.StartupInfo, &info);
    const DWORD createError = GetLastError();
    DeleteProcThreadAttributeList(attributes);
    CloseHandle(input);
    CloseHandle(outWrite);
    CloseHandle(errWrite);
    if (!created) {
        result.error = std::system_category().message(static_cast<int>(createError));
        CloseHandle(outRead);
        CloseHandle(errRead);
        return result;
    }
    CloseHandle(info.hThread);
    result.started = true;

    for (;;) {
        drain(outRead, ProcessChannel::StdOut, options, result, buffer);
        drain(errRead, ProcessChannel::StdErr, options, result, buffer);
        if (WaitForSingleObject(info.hProcess, static_cast<DWORD>(kPoll.count())) == WAIT_OBJECT_0)
            break;
        if (deadline && Clock::now() >= *deadline) {
            TerminateProcess(info.hProcess, 1);
            WaitForSingleObject(info.hProcess, INFINITE);
            result.timed_out = true;
            break;
        }
    }
    drain(outRead, ProcessChannel::StdOut, options, result, buffer);
    drain(errRead, ProcessChannel::StdErr, options, result, buffer);
    for (HANDLE h : {outRead, errRead}) {
        if (h != nullptr)
            CloseHandle(h);
    }
    DWORD exitCode = 0;
    GetExitCodeProcess(info.hProcess, &exitCode);
    CloseHandle(info.hProcess);
    if (!result.timed_out)
        result.exit_code = static_cast<int>(exitCode);
    return result;

#else
    std::array<int, 2> out{-1, -1};
    std::array<int, 2> err{-1, -1};
    if (!make_pipe(out) || !make_pipe(err)) {
        result.error = std::generic_category().message(errno);
        for (const int fd : {out[0], out[1], err[0], err[1]}) {
            if (fd >= 0)
                close(fd);
        }
        return result;
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_adddup2(&actions, out[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, err[1], STDERR_FILENO);
    const std::string workingDir = options.working_dir.string();
    if (!workingDir.empty())
        add_chdir(actions, workingDir.c_str());

    const std::string path = program.string();
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(path.c_str())); // NOLINT(cppcoreguidelines-pro-type-const-cast): posix_spawn takes char* const[]
    for (const std::string& arg : args)
        argv.push_back(const_cast<char*>(arg.c_str())); // NOLINT(cppcoreguidelines-pro-type-const-cast): posix_spawn takes char* const[]
    argv.push_back(nullptr);

    std::vector<std::string> environment;
    std::vector<char*> envp;
    char** env = environ;
    if (!options.environment.empty()) {
        for (char** p = environ; *p != nullptr; ++p) {
            const std::string_view entry(*p);
            const bool replaced = std::ranges::any_of(options.environment, [&](const auto& kv) {
                return entry.size() > kv.first.size() && entry.starts_with(kv.first) && entry[kv.first.size()] == '=';
            });
            if (!replaced)
                environment.emplace_back(entry);
        }
        for (const auto& [key, value] : options.environment)
            environment.push_back(std::format("{}={}", key, value));
        envp.reserve(environment.size() + 1);
        for (std::string& entry : environment)
            envp.push_back(entry.data());
        envp.push_back(nullptr);
        env = envp.data();
    }

    pid_t pid = 0;
    const int spawned = posix_spawnp(&pid, path.c_str(), &actions, nullptr, argv.data(), env);
    posix_spawn_file_actions_destroy(&actions);
    close(out[1]);
    close(err[1]);
    if (spawned != 0) {
        result.error = std::generic_category().message(spawned);
        close(out[0]);
        close(err[0]);
        return result;
    }
    result.started = true;

    std::array<int, 2> fds{out[0], err[0]};
    int status = 0;
    bool exited = false;
    while (!exited) {
        std::array<pollfd, 2> polled{pollfd{fds[0], POLLIN, 0}, pollfd{fds[1], POLLIN, 0}};
        // Poll in short steps: a grandchild that inherited the pipes must not hold us after the child exits
        if (poll(polled.data(), polled.size(), static_cast<int>(kPoll.count())) > 0) {
            for (std::size_t i = 0; i < fds.size(); ++i) {
                if (fds[i] >= 0 && (polled[i].revents & (POLLIN | POLLHUP | POLLERR)) != 0)
                    drain(fds[i], i == 0 ? ProcessChannel::StdOut : ProcessChannel::StdErr, options, result, buffer);
            }
        }
        exited = waitpid(pid, &status, WNOHANG) == pid;
        if (!exited && deadline && Clock::now() >= *deadline) {
            kill(pid, SIGKILL);
            while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
            }
            result.timed_out = true;
            break;
        }
    }
    for (int& fd : fds) {
        if (fd < 0)
            continue;
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        while (fd >= 0) {
            const ssize_t n = read(fd, buffer.data(), buffer.size());
            if (n <= 0) {
                close(fd);
                fd = -1;
                break;
            }
            deliver(options, result, &fd == fds.data() ? ProcessChannel::StdOut : ProcessChannel::StdErr, std::string_view(buffer.data(), static_cast<std::size_t>(n)));
        }
    }
    if (result.timed_out)
        return result;
    if (WIFEXITED(status))
        result.exit_code = WEXITSTATUS(status);
    else
        result.crashed = WIFSIGNALED(status);
    return result;
#endif
}

//=============================================================================================================

std::string mne::host_name()
{
#if defined(_WIN32)
    std::array<wchar_t, 256> name{};
    DWORD size = static_cast<DWORD>(name.size());
    if (!GetComputerNameExW(ComputerNameDnsHostname, name.data(), &size))
        return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, name.data(), static_cast<int>(size), nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, name.data(), static_cast<int>(size), utf8.data(), n, nullptr, nullptr);
    return utf8;
#elif defined(__EMSCRIPTEN__)
    return {};
#else
    std::array<char, 256> name{};
    if (gethostname(name.data(), name.size()) != 0)
        return {};
    name.back() = '\0';
    return name.data();
#endif
}

//=============================================================================================================

std::string mne::os_pretty_name()
{
#if defined(_WIN32)
    struct VersionInfo
    {
        ULONG size;
        ULONG major;
        ULONG minor;
        ULONG build;
        ULONG platform;
        std::array<WCHAR, 128> servicePack;
    };
    // RtlGetVersion reports the real version; GetVersionEx is capped by the application manifest
    using RtlGetVersion = LONG(WINAPI*)(VersionInfo*);
    VersionInfo version{sizeof(VersionInfo), 0, 0, 0, 0, {}};
    if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
        if (auto get = reinterpret_cast<RtlGetVersion>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion"))))
            get(&version);
    }
    const ULONG product = version.major == 10 && version.build >= 22000 ? 11 : version.major;
    return std::format("Windows {} ({}.{}.{})", product, version.major, version.minor, version.build);
#elif defined(__APPLE__)
    std::array<char, 64> version{};
    std::size_t size = version.size();
    if (sysctlbyname("kern.osproductversion", version.data(), &size, nullptr, 0) != 0)
        return "macOS";
    return std::string("macOS ") + version.data();
#elif defined(__EMSCRIPTEN__)
    return "WebAssembly";
#else
    for (const char* file : {"/etc/os-release", "/usr/lib/os-release"}) {
        std::ifstream in(file);
        for (std::string line; std::getline(in, line);) {
            if (!line.starts_with("PRETTY_NAME="))
                continue;
            std::string_view value = std::string_view(line).substr(12);
            if (value.size() >= 2 && (value.front() == '"' || value.front() == '\'') && value.back() == value.front())
                value = value.substr(1, value.size() - 2);
            return std::string(value);
        }
    }
    utsname system{};
    if (uname(&system) != 0)
        return {};
    return std::string(system.sysname) + ' ' + system.release;
#endif
}

//=============================================================================================================

std::string_view mne::cpu_architecture()
{
#if defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "i386";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__wasm__)
    return "wasm";
#elif defined(__riscv) && __riscv_xlen == 64
    return "riscv64";
#else
    return "unknown";
#endif
}

//=============================================================================================================

std::vector<HardwareAddress> mne::hardware_addresses()
{
    std::vector<HardwareAddress> addresses;
    const auto add = [&addresses](const unsigned char* bytes, std::size_t length) {
        if (length != 6)
            return;
        HardwareAddress address{};
        std::copy_n(bytes, address.size(), address.begin());
        if (std::ranges::any_of(address, [](std::uint8_t b) { return b != 0; }))
            addresses.push_back(address);
    };
#if defined(_WIN32)
    ULONG size = 16 * 1024;
    std::vector<unsigned char> buffer;
    ULONG status = ERROR_BUFFER_OVERFLOW;
    for (int attempt = 0; attempt < 3 && status == ERROR_BUFFER_OVERFLOW; ++attempt) {
        buffer.resize(size);
        status = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
    }
    if (status != NO_ERROR)
        return addresses;
    for (const IP_ADAPTER_ADDRESSES* adapter = reinterpret_cast<const IP_ADAPTER_ADDRESSES*>(buffer.data()); adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->IfType == IF_TYPE_PPP)
            continue;
        add(adapter->PhysicalAddress, adapter->PhysicalAddressLength);
    }
#elif defined(__APPLE__) || defined(__linux__)
    ifaddrs* list = nullptr;
    if (getifaddrs(&list) != 0)
        return addresses;
    for (const ifaddrs* entry = list; entry; entry = entry->ifa_next) {
        const unsigned int flags = entry->ifa_flags;
        if (!entry->ifa_addr || !(flags & IFF_UP) || !(flags & IFF_RUNNING) || (flags & IFF_LOOPBACK) || (flags & IFF_POINTOPOINT))
            continue;
#if defined(__APPLE__)
        if (entry->ifa_addr->sa_family == AF_LINK) {
            const auto* link = reinterpret_cast<const sockaddr_dl*>(entry->ifa_addr);
            add(reinterpret_cast<const unsigned char*>(LLADDR(link)), link->sdl_alen);
        }
#else
        if (entry->ifa_addr->sa_family == AF_PACKET) {
            const auto* link = reinterpret_cast<const sockaddr_ll*>(entry->ifa_addr);
            add(link->sll_addr, link->sll_halen);
        }
#endif
    }
    freeifaddrs(list);
#endif
    return addresses;
}
