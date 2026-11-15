//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     python_test_helper.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::PythonTestHelper: package probes and @c eval helpers.
 *
 * Numeric output is parsed with mne::read_matrix, so stdout, files written by @c numpy.savetxt and files written
 * by mne::write_matrix all follow the same rules.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "python_test_helper.h"

#include <base/string_utils.h>
#include <math/matrix_io.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstdlib>
#include <sstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool PythonTestHelper::is_available() const
{
    return is_python_available() && has_package("mne");
}

//=============================================================================================================

bool PythonTestHelper::is_python_available() const
{
    return m_runner.is_python_available();
}

//=============================================================================================================

bool PythonTestHelper::has_package(std::string_view package) const
{
    return m_runner.is_package_available(package);
}

//=============================================================================================================

PythonRunner PythonTestHelper::runner(std::chrono::milliseconds timeout) const
{
    PythonRunnerConfig config = m_runner.config();
    config.timeout = timeout;
    return PythonRunner(std::move(config));
}

//=============================================================================================================

PythonRunnerResult PythonTestHelper::eval(std::string_view code, std::chrono::milliseconds timeout) const
{
    return runner(timeout).run_code(code);
}

//=============================================================================================================

std::optional<double> PythonTestHelper::eval_double(std::string_view code, std::chrono::milliseconds timeout) const
{
    const PythonRunnerResult result = eval(code, timeout);
    return result.success ? to_double(trimmed(result.std_out)) : std::nullopt;
}

//=============================================================================================================

std::optional<VectorXd> PythonTestHelper::eval_vector(std::string_view code, std::chrono::milliseconds timeout) const
{
    const PythonRunnerResult result = eval(code, timeout);
    if (!result.success) {
        return std::nullopt;
    }
    // Values may be spread over lines of different length, so read them as one row
    std::string flat = result.std_out;
    std::ranges::replace(flat, '\n', ' ');
    std::istringstream stream(flat);
    RowVectorXd values;
    if (!mne::read_matrix(values, stream) || values.size() == 0) {
        return std::nullopt;
    }
    return values.transpose();
}

//=============================================================================================================

std::optional<MatrixXd> PythonTestHelper::eval_matrix(std::string_view code, std::chrono::milliseconds timeout) const
{
    const PythonRunnerResult result = eval(code, timeout);
    if (!result.success) {
        return std::nullopt;
    }
    std::istringstream stream(result.std_out);
    MatrixXd matrix;
    if (!mne::read_matrix(matrix, stream) || matrix.size() == 0) {
        return std::nullopt;
    }
    return matrix;
}

//=============================================================================================================

std::optional<MatrixXd> PythonTestHelper::eval_matrix_via_file(std::string_view code,
                                                               const std::filesystem::path& file,
                                                               std::chrono::milliseconds timeout) const
{
    if (!eval(code, timeout).success) {
        return std::nullopt;
    }
    MatrixXd matrix;
    if (!mne::read_matrix(matrix, file) || matrix.size() == 0) {
        return std::nullopt;
    }
    return matrix;
}

//=============================================================================================================

PythonRunnerResult PythonTestHelper::run_script(const std::filesystem::path& script,
                                                const StringList& args,
                                                std::chrono::milliseconds timeout) const
{
    return runner(timeout).run(script, args);
}

//=============================================================================================================

std::filesystem::path PythonTestHelper::test_data_path()
{
    return executable_dir().parent_path() / "resources" / "data" / "mne-cpp-test-data";
}

//=============================================================================================================

bool PythonTestHelper::is_python_required()
{
    const char* value = std::getenv("MNE_REQUIRE_PYTHON"); // NOLINT(concurrency-mt-unsafe): read-only, no setenv here
    return value != nullptr && (iequals(value, "true") || std::string_view(value) == "1");
}
