//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Base library: logging, byte streams, string and path helpers, JSON, CBOR, SHA-256, processes, the thread pool and the circular buffer.
 *
 * Every step checks its result and the example exits non-zero on a mismatch.
 * All files are written to a temporary directory.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/byte_stream.h>
#include <base/cbor.h>
#include <base/circular_buffer.h>
#include <base/json.h>
#include <base/json_stream_parser.h>
#include <base/log.h>
#include <base/process.h>
#include <base/sha256.h>
#include <base/string_utils.h>
#include <base/thread_pool.h>
#include <base/xml.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, std::string_view what)
{
    std::println("{} {}", condition ? "  ok  " : "  FAIL", what);
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "mne_cpp_ex_base";
    std::filesystem::create_directories(dir);
    bool ok = true;

    //! [log_file_usage]
    Log::set_file(dir / "example.log"); // printed to the console, and a plain-text copy here
    Log::info("logging to {}", utf8_from_path(Log::file()));
    //! [log_file_usage]
    Log::set_file({});
    std::string logText;
    {
        std::ifstream logFile(dir / "example.log"); // closed before the directory is removed, which Windows refuses while it is open
        logText.assign(std::istreambuf_iterator<char>(logFile), {});
    }
    ok &= expect(logText.find("logging to") != std::string::npos, "Log wrote the message to the log file");

    //! [log_usage]
    Log::warning("channel {} is flat", "MEG 0113"); // formatted as by std::format
    //! [log_usage]

    //! [log_capture_usage]
    LogCapture capture; // in a test: record instead of printing
    Log::warning("channel MEG 0113 is flat");
    const int flat = capture.count(Log::Level::Warning, "flat");
    //! [log_capture_usage]
    ok &= expect(flat == 1 && capture.entries().size() == 1, "LogCapture recorded the warning");

    //! [byte_stream_usage]
    const std::vector<float> samples = {1.5f, -0.25f};
    {
        auto out = FileByteStream::open(dir / "data.bin", FileByteStream::Mode::Write);
        out->write_big_endian(std::int32_t(samples.size())); // FIFF byte order
        out->write_big_endian(samples.data(), samples.size());
    }
    auto in = FileByteStream::open(dir / "data.bin", FileByteStream::Mode::Read);
    std::int32_t count = 0;
    in->read_big_endian(count);
    std::vector<float> back(static_cast<std::size_t>(count));
    in->read_big_endian(back.data(), back.size());
    //! [byte_stream_usage]
    ok &= expect(back == samples && in->at_end(), "FileByteStream read back the big-endian samples");
    in.reset(); // Windows does not remove an open file

    //! [memory_byte_stream_usage]
    MemoryByteStream memory;
    memory.write_big_endian(std::int16_t{0x0102});
    const bool bigEndian = memory.data()[0] == std::byte{0x01}; // most significant byte first
    //! [memory_byte_stream_usage]
    ok &= expect(bigEndian && memory.size() == 2, "MemoryByteStream stores values most significant byte first");

    //! [thread_pool_usage]
    const std::vector<int> channels = {0, 1, 2, 3};
    const std::vector<double> power = ThreadPool::global().parallel_map(channels, [](int ch) { return 0.5 * ch; }); // in order
    auto background = ThreadPool::global().submit([] { return 42; });
    //! [thread_pool_usage]
    ok &= expect(power == std::vector<double>({0.0, 0.5, 1.0, 1.5}) && background.get() == 42, "ThreadPool mapped the channels in order");

    //! [string_utils_usage]
    const StringList fields = split(trimmed(" MEG 0113, 1.5e-13 \n"), ','); // a line of a text file
    const std::optional<double> scale = to_double(fields[1]);               // locale independent
    const bool isGz = iequals(utf8_from_path(path_from_utf8("T1.NII.GZ").extension()), ".gz");
    //! [string_utils_usage]
    ok &= expect(fields[0] == "MEG 0113" && scale == 1.5e-13 && isGz, "string helpers parsed the line");

    //! [json_usage]
    // Qt:   QJsonParseError error;
    //       QJsonObject cfg = QJsonDocument::fromJson(bytes, &error).object();
    //       int rate = cfg.value("sfreq").toInt(1000);
    //       cfg["channels"] = QJsonArray{"MEG 0113", "EEG 001"};
    //       QByteArray out = QJsonDocument(cfg).toJson(QJsonDocument::Compact);
    JsonParseError parseError;
    JsonObject cfg = JsonDocument::from_json(R"({"sfreq": 600, "device": "neuromag"})", &parseError).object();
    const int rate = cfg.value("sfreq").to_int(1000);       // safe fallback on a missing key or wrong type
    const bool hasGain = !cfg.value("gain").is_undefined(); // missing keys are undefined; non-const [] inserts, as in Qt
    cfg["channels"] = JsonArray{"MEG 0113", "EEG 001"};
    const std::string out = JsonDocument(cfg).to_json(JsonDocument::Compact); // keys sorted, like Qt
    //! [json_usage]
    ok &= expect(parseError.error == JsonParseError::NoError && rate == 600 && !hasGain && out == R"({"channels":["MEG 0113","EEG 001"],"device":"neuromag","sfreq":600})",
                 "JsonDocument parsed, edited and wrote the configuration");

    //! [xml_usage]
    // Qt:   QXmlStreamReader xml(&file);
    //       while (!xml.atEnd()) if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == u"sensor") ...
    const Expected<XmlElement> layout = parse_xml(R"(<sensors><sensor id="1"><name>Fp1</name><x>-27.5</x></sensor></sensors>)");
    std::string firstSensor;
    if (layout) {
        for (const XmlElement& sensor : layout->children) // XmlElement::child() finds the first one by local name
            if (sensor.local_name() == "sensor" && sensor.child("name"))
                firstSensor = firstSensor.empty() ? sensor.child("name")->text : firstSensor;
    } else {
        Log::warning("{}", layout.error().what()); // the line where the document is not well formed
    }
    //! [xml_usage]
    ok &= expect(firstSensor == "Fp1" && layout && *layout->children.front().attribute("id") == "1", "parse_xml read the sensor layout");

    //! [cbor_usage]
    // Qt:   QCborMap header;
    //       header["sfreq"] = 600;
    //       header["payload"] = QByteArray(raw, 3);
    //       QByteArray encoded = QCborValue(header).toCbor();
    //       QCborValue decoded = QCborValue::fromCbor(encoded, &error);
    CborMap header{{"sfreq", 600}, {"device", "neuromag"}};
    header["payload"] = CborByteArray{std::byte{1}, std::byte{2}, std::byte{3}}; // binary stays binary
    const CborByteArray encoded = CborValue(header).to_cbor();                   // same bytes as QCborValue::toCbor()
    CborParseError cborError;
    const CborValue decoded = CborValue::from_cbor(encoded, &cborError);
    const std::int64_t sfreq = decoded["sfreq"].to_integer();
    const JsonValue asJson = decoded.to_json_value(); // byte strings become base64url text
    //! [cbor_usage]
    ok &= expect(cborError.error == CborParseError::NoError && sfreq == 600 && decoded["payload"].to_byte_array().size() == 3 && asJson["payload"].to_string() == "AQID",
                 "CborValue encoded and decoded a header with a byte string");

    //! [json_stream_parser_usage]
    JsonStreamParser stream; // NDJSON from a socket, split anywhere
    std::vector<double> latencies;
    stream.set_document_callback([&](JsonDocument&& event) { latencies.push_back(event["latency"].to_double()); });
    for (const std::string_view packet : {"{\"latency\":1.5}\n{\"lat", "ency\":2", ".25}\n{\"latency\":3}"}) {
        if (!stream.feed(packet)) { // malformed input: report and resynchronize
            Log::warning("bad JSON at byte {}: {}", stream.error().offset, stream.error().error_string());
            stream.reset();
        }
    }
    //! [json_stream_parser_usage]
    ok &= expect(latencies == std::vector<double>({1.5, 2.25, 3.0}), "JsonStreamParser emitted each event as it closed");

    //! [sha256_usage]
    // Qt:   QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex()
    const std::string fingerprint = to_hex(Sha256::hash("abc"));
    Sha256 incremental; // or feed a large file block by block
    incremental.update("a").update("bc");
    //! [sha256_usage]
    ok &= expect(fingerprint == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" && incremental.digest() == Sha256::hash("abc"),
                 "Sha256 hashed a message in one call and incrementally");

    //! [process_usage]
    // Qt:   QProcess p; p.start("python3", {"--version"}); p.waitForFinished(5000); p.readAllStandardOutput();
    ProcessOptions options;
    options.timeout = std::chrono::seconds(5); // killed if it takes longer
    const ProcessResult python = run_process("python3", std::vector<std::string>{"--version"}, options);
    Log::info("{} on {} ({}): {}", host_name(), os_pretty_name(), cpu_architecture(), python.ok() ? python.std_out : python.error);
    Log::info("{} network interface(s) with a hardware address", hardware_addresses().size());
    //! [process_usage]
    ok &= expect(!host_name().empty() && !os_pretty_name().empty() && (python.started || !python.error.empty()),
                 "run_process ran a program and the host was described");

    //! [circular_buffer_usage]
    CircularBuffer<double> buffer(4); // one thread pushes, another pops
    buffer.push(std::array{1.0, 2.0, 3.0});
    double first = 0.0;
    double second = 0.0;
    buffer.pop(first);
    buffer.pop(second);
    //! [circular_buffer_usage]
    ok &= expect(first == 1.0 && second == 2.0 && buffer.size() == 1, "CircularBuffer pops in FIFO order, one element left");

    std::error_code removeError;
    std::filesystem::remove_all(dir, removeError);
    ok &= expect(!removeError, std::format("the example's files were removed{}", removeError ? ": " + removeError.message() : std::string()));
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
