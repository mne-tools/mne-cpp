//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_graph_executor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref MnaGraphExecutor — batch / incremental graph walk plus MNE-Scan-friendly stream-mode plugin wiring.
 *
 * @ref MnaGraphExecutor::execute first validates the graph, then
 * runs @ref MnaGraph::topological_sort and visits every node in
 * order. For each node it gathers inputs from the
 * @ref Context::results map keyed by @c srcNodeId::srcPortName,
 * falls back to @ref Context::graph_inputs for graph-level entry
 * ports, looks the @c op_type up in @ref MnaOpRegistry, invokes
 * the registered @ref MnaOpRegistry::OpFunc with the merged
 * inputs and node attributes, and writes the returned outputs
 * back into the context map. @ref execute_incremental reuses the
 * same loop but restricts it to nodes returned by
 * @ref MnaGraph::dirty_nodes and their @ref MnaGraph::downstream_nodes.
 *
 * @ref start_stream / @ref stop_stream provide the MNE Scan
 * integration point: the host application supplies a
 * @ref PluginFactory that maps @c op_type strings to live
 * @c QObject plugin instances; the executor instantiates one per
 * node in topological order, applies @ref MnaParamTree values to
 * their attributes, and stores the resulting @ref StreamContext
 * so the host can wire signal/slot connections by the matching
 * @ref MnaDataKind on each port and tear down the pipeline in
 * reverse order on stop.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_graph_executor.h"

#include "mna_graph.h"
#include "mna_op_registry.h"

#include <mne/base/log.h>
#include <mne/base/process.h>
#include <mne/base/sha256.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <random>
#include <set>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

MnaGraphExecutor::ProgressCallback& progressCallback()
{
    static MnaGraphExecutor::ProgressCallback callback;
    return callback;
}

UtcTime now()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

/** @return A number, boolean, text or JSON value held by @p value as JSON; nothing for other objects. */
std::optional<JsonValue> scalar(const std::any& value)
{
    if (const auto* v = std::any_cast<JsonValue>(&value))
        return *v;
    if (const auto* v = std::any_cast<double>(&value))
        return JsonValue(*v);
    if (const auto* v = std::any_cast<float>(&value))
        return JsonValue(*v);
    if (const auto* v = std::any_cast<int>(&value))
        return JsonValue(*v);
    if (const auto* v = std::any_cast<std::int64_t>(&value))
        return JsonValue(*v);
    if (const auto* v = std::any_cast<bool>(&value))
        return JsonValue(*v);
    if (const auto* v = std::any_cast<std::string>(&value))
        return JsonValue(*v);
    if (const auto* v = std::any_cast<const char*>(&value))
        return JsonValue(*v);
    return std::nullopt;
}

/** @return The scalar values of @p values as a JSON object. */
JsonObject scalars(const MnaValues& values)
{
    JsonObject json;
    for (const auto& [key, value] : values) {
        if (std::optional<JsonValue> v = scalar(value)) {
            json.insert(key, std::move(*v));
        }
    }
    return json;
}

/** @return The text of a scalar JSON value as it is substituted for a placeholder: 3.0 gives "3". */
std::string text(const JsonValue& value)
{
    if (value.is_string())
        return std::string(value.to_string());
    if (value.is_bool())
        return value.to_bool() ? "true" : "false";
    if (value.is_integer())
        return std::to_string(value.to_integer());
    if (value.is_number())
        return std::format("{}", value.to_double());
    return {};
}

/** @return The text of a run-time value; empty for objects that are not scalar. */
std::string text(const std::any& value)
{
    const std::optional<JsonValue> v = scalar(value);
    return v ? text(*v) : std::string();
}

/** Replaces every {{name}} in @p subject by the text of the input, else attribute, called name. */
std::string substitute(std::string subject, const MnaValues& inputs, const JsonObject& attributes)
{
    for (const auto& [key, value] : inputs) {
        for (std::size_t at = 0; (at = subject.find("{{" + key + "}}", at)) != std::string::npos;) {
            const std::string replacement = text(value);
            subject.replace(at, key.size() + 4, replacement);
            at += replacement.size();
        }
    }
    for (const auto& [key, value] : attributes) {
        for (std::size_t at = 0; (at = subject.find("{{" + key + "}}", at)) != std::string::npos;) {
            const std::string replacement = text(value);
            subject.replace(at, key.size() + 4, replacement);
            at += replacement.size();
        }
    }
    return subject;
}

/** Runs @p program and returns stdout, stderr and the exit code (-1 if it did not run to completion). */
MnaValues runProgram(const std::string& program, const StringList& args, const std::string& work_dir = {})
{
    ProcessOptions options;
    options.working_dir = path_from_utf8(work_dir);
    ProcessResult process = run_process(path_from_utf8(program), args, options);

    MnaValues outputs;
    outputs["stdout"] = std::move(process.std_out);
    outputs["stderr"] = process.started ? std::move(process.std_err) : std::move(process.error);
    outputs["exit_code"] = process.exit_code;
    return outputs;
}

/** File extension and default interpreter of a script language. */
struct Language
{
    std::string_view name;
    std::string_view extension;
    std::string_view interpreter;
};

constexpr std::array<Language, 6> kLanguages{{
    {"python", ".py", "python3"},
    {"shell", ".sh", "/bin/bash"},
    {"r", ".R", "Rscript"},
    {"matlab", ".m", "matlab"},
    {"octave", ".m", "octave"},
    {"julia", ".jl", "julia"},
}};

/** Writes the script to a temporary file, runs its interpreter on it and removes it unless asked to keep it. */
MnaValues runScript(const MnaScript& script, const MnaValues& inputs, const JsonObject& attributes)
{
    const auto language = std::ranges::find(kLanguages, std::string_view(script.language), &Language::name);
    const std::string_view extension = language != kLanguages.end() ? language->extension : ".txt";
    std::string interpreter = script.interpreter;
    if (interpreter.empty() && language != kLanguages.end()) {
        interpreter = language->interpreter;
    }

    std::random_device random;
    std::error_code ec;
    const std::filesystem::path file =
        std::filesystem::temp_directory_path(ec) / std::format("mna_script_{:08x}{:08x}{}", random(), random(), extension);
    std::ofstream(file, std::ios::binary) << substitute(script.code, inputs, attributes);
    if (!std::filesystem::exists(file, ec)) {
        return {{"stderr", std::string("Failed to create temporary script file")}, {"exit_code", -1}};
    }

    StringList args = script.interpreter_args;
    args.push_back(utf8_from_path(file));
    MnaValues outputs = runProgram(interpreter, args);
    if (!script.keep_temp_file) {
        std::filesystem::remove(file, ec);
    }
    return outputs;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaGraphExecutor::Context MnaGraphExecutor::execute(MnaGraph& graph, const MnaValues& graph_inputs)
{
    Context context;
    context.graph_inputs = graph_inputs;
    for (const auto& [port, value] : graph_inputs) {
        context.results.emplace("graph::" + port, value);
    }

    graph.param_tree.evaluate(scalars(context.results));
    apply_param_tree(graph);

    const StringList order = graph.topological_sort();
    const int total = static_cast<int>(order.size());
    for (int i = 0; i < total; ++i) {
        const std::string& id = order[static_cast<std::size_t>(i)];
        if (progressCallback()) {
            progressCallback()(id, i + 1, total);
        }
        if (!run_node(graph, graph.node(id), context)) {
            break;
        }
    }

    graph.param_tree.evaluate(scalars(context.results)); // on_change bindings see the new results
    return context;
}

//=============================================================================================================

void MnaGraphExecutor::execute_incremental(MnaGraph& graph, Context& context)
{
    std::set<std::string, std::less<>> pending;
    for (const std::string& id : graph.dirty_nodes()) {
        pending.insert(id);
        for (std::string& downstream : graph.downstream_nodes(id)) {
            pending.insert(std::move(downstream));
        }
    }

    apply_param_tree(graph);

    StringList order;
    for (std::string& id : graph.topological_sort()) {
        if (pending.contains(id)) {
            order.push_back(std::move(id));
        }
    }
    const int total = static_cast<int>(order.size());
    for (int i = 0; i < total; ++i) {
        const std::string& id = order[static_cast<std::size_t>(i)];
        if (progressCallback()) {
            progressCallback()(id, i + 1, total);
        }
        if (!run_node(graph, graph.node(id), context)) {
            break;
        }
    }

    graph.param_tree.evaluate(scalars(context.results));
}

//=============================================================================================================

void MnaGraphExecutor::apply_param_tree(MnaGraph& graph)
{
    for (const std::string& path : graph.param_tree.all_paths()) {
        const std::size_t sep = path.find('/');
        if (sep == 0 || sep == std::string::npos) {
            continue;
        }
        if (MnaNode* node = graph.find_node(std::string_view(path).substr(0, sep))) {
            node->attributes.insert(std::string_view(path).substr(sep + 1), graph.param_tree.param(path));
        }
    }
}

//=============================================================================================================

bool MnaGraphExecutor::run_node(MnaGraph& graph, MnaNode& node, Context& context)
{
    MnaValues inputs;
    for (const MnaPort& p : node.inputs) {
        if (p.source_node_id.empty()) {
            continue;
        }
        const auto it = context.results.find(p.source_node_id + "::" + p.source_port_name);
        inputs[p.name] = it != context.results.end() ? it->second : std::any();
    }
    JsonObject scope = node.attributes; // checks see attributes and inputs by name, post checks the outputs too
    for (auto& [key, value] : scalars(inputs)) {
        scope.insert(key, value);
    }

    MnaVerification& verification = node.verification;
    MnaProvenance& provenance = verification.provenance;
    provenance = MnaProvenance();
    provenance.mne_cpp_version = MNE_CPP_VERSION;
    provenance.os_info = os_pretty_name() + ' ' + std::string(cpu_architecture());
    provenance.host_name = host_name();
    provenance.resolved_attributes = node.attributes;
    for (const auto& [port, value] : inputs) {
        provenance.input_hashes.emplace(port, to_hex(Sha256::hash(text(value))));
    }

    const auto failsHard = [](const std::vector<MnaVerificationResult>& results) {
        return std::ranges::any_of(results, [](const MnaVerificationResult& r) {
            return !r.passed && r.severity == "error";
        });
    };
    verification.pre_results = run_checks(graph, node, "pre", scope);
    verification.post_results.clear();
    if (failsHard(verification.pre_results)) {
        context.aborted_node = node.id;
        return false;
    }

    provenance.started_at = now();
    const auto start = std::chrono::steady_clock::now();
    MnaValues outputs = execute_node(node, inputs);
    provenance.wall_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    provenance.finished_at = now();

    for (auto& [key, value] : scalars(outputs)) {
        scope.insert(key, value);
    }
    for (auto& [port, value] : outputs) {
        context.results.insert_or_assign(node.id + "::" + port, std::move(value));
    }
    node.dirty = false;
    node.executed_at = provenance.finished_at;

    verification.post_results = run_checks(graph, node, "post", scope);
    if (failsHard(verification.post_results)) {
        context.aborted_node = node.id;
        return false;
    }
    return true;
}

//=============================================================================================================

std::vector<MnaVerificationResult> MnaGraphExecutor::run_checks(const MnaGraph& graph, const MnaNode& node, std::string_view phase, const JsonObject& scope)
{
    std::vector<MnaVerificationResult> results;
    for (const MnaVerificationCheck& check : node.verification.checks) {
        if (check.phase != phase) {
            continue;
        }
        MnaVerificationResult result;
        result.check_id = check.id;
        result.severity = check.severity;
        result.evaluated_at = now();
        if (!check.script.code.empty()) {
            MnaValues values;
            for (const auto& [key, value] : scope) {
                values.emplace(key, value);
            }
            const MnaValues out = runScript(check.script, values, node.attributes);
            const int exitCode = std::any_cast<int>(out.at("exit_code"));
            result.actual_value = exitCode;
            result.passed = exitCode == 0;
        } else {
            result.actual_value = graph.param_tree.evaluate_expression(check.expression, scope);
            result.passed = result.actual_value.is_bool() && result.actual_value.to_bool();
        }
        result.message = result.passed ? std::format("PASS: {}", check.description) : std::format("FAIL [{}]: {}", check.severity, check.description);
        if (!result.passed && !check.on_fail.empty()) {
            result.message += std::format(" ({})", check.on_fail);
        }
        results.push_back(std::move(result));
    }
    return results;
}

//=============================================================================================================

MnaValues MnaGraphExecutor::execute_node(const MnaNode& node, const MnaValues& inputs)
{
    if (node.exec_mode == MnaNodeExecMode::Script) {
        return runScript(node.script, inputs, node.attributes);
    }
    if (node.exec_mode == MnaNodeExecMode::Ipc) {
        StringList args;
        args.reserve(node.ipc_args.size());
        for (const std::string& arg : node.ipc_args) {
            args.push_back(substitute(arg, inputs, node.attributes));
        }
        MnaValues outputs = runProgram(node.ipc_command, args, node.ipc_work_dir);
        for (const MnaPort& p : node.outputs) {
            if (!p.cached_result_path.empty()) {
                outputs[p.name] = p.cached_result_path;
            }
        }
        return outputs;
    }
    if (const MnaOpRegistry::OpFunc& func = MnaOpRegistry::instance().op_func(node.op_type)) {
        return func(inputs, node.attributes);
    }
    return {}; // no implementation registered
}

//=============================================================================================================

void MnaGraphExecutor::set_progress_callback(ProgressCallback callback)
{
    progressCallback() = std::move(callback);
}

//=============================================================================================================

MnaGraphExecutor::StreamContext MnaGraphExecutor::start_stream(MnaGraph& graph, const PluginFactory& factory)
{
    StreamContext context;
    context.graph = &graph;

    if (const StringList errors = graph.validate(); !errors.empty()) {
        Log::warning("MnaGraphExecutor::start_stream - graph validation failed: {}", join(errors, ", "));
        return context;
    }
    context.execution_order = graph.topological_sort();
    apply_param_tree(graph);

    for (const std::string& id : context.execution_order) {
        const MnaNode& n = graph.node(id);
        std::shared_ptr<void> plugin = factory(n.op_type);
        if (!plugin) {
            Log::warning("MnaGraphExecutor::start_stream - factory returned nullptr for op type: {}", n.op_type);
            context.live_plugins.clear();
            return context;
        }
        context.live_plugins[id] = std::move(plugin);
    }
    context.running = true; // wiring the plugins together is up to the host application
    return context;
}

//=============================================================================================================

void MnaGraphExecutor::stop_stream(StreamContext& context)
{
    if (!context.running) {
        return;
    }
    context.running = false;
    for (auto it = context.execution_order.rbegin(); it != context.execution_order.rend(); ++it) {
        context.live_plugins.erase(*it);
    }
    context.live_plugins.clear();
    context.execution_order.clear();
    context.graph = nullptr;
}
