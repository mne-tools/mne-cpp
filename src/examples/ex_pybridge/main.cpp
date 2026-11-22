//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Python bridge: run a script with arguments and progress reporting, and read a matrix NumPy prints.
 *
 * Every step checks its result and the example exits non-zero on a mismatch. Without a @c python3 on PATH the
 * Python steps are reported as skipped.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/pybridge/python_runner.h>
#include <mne/pybridge/python_test_helper.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <format>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

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

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    ok &= expect(parse_progress_line("[progress] 42.5% epoch 3") == std::make_pair(42.5f, std::string("epoch 3")),
                 "progress lines are parsed without starting Python");

    //! [python_runner_usage]
    PythonRunner python; // "python3" from PATH; a timeout or venv is set via PythonRunnerConfig
    std::vector<std::string> progress;
    python.set_progress_callback([&](float pct, std::string_view message) { progress.push_back(std::format("{} {}", pct, message)); });
    const PythonRunnerResult run = python.run_code("import sys\nprint('[progress] 50% half')\nprint(sum(map(int, sys.argv[1:])))", {"2", "3"});
    //! [python_runner_usage]
    //! [python_test_helper_usage]
    PythonTestHelper helper;                                                                    // oracle access for tests
    const std::optional<MatrixXd> fromNumpy = helper.eval_matrix("print('1 2'); print('3 4')"); // one line per row
    //! [python_test_helper_usage]
    if (python.is_python_available()) {
        ok &= expect(run.success && run.std_out.ends_with("5") && progress == std::vector<std::string>{"50 half"},
                     "PythonRunner passes arguments and parses [progress] lines");
        ok &= expect(fromNumpy && *fromNumpy == (Matrix2d() << 1, 2, 3, 4).finished(), "PythonTestHelper reads a matrix");
    } else {
        std::println("  skip  PythonRunner (python3 not found)");
    }

    std::println("{}", ok ? "All pybridge checks passed." : "pybridge checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
