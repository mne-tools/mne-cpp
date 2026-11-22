//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mna_graph_execution.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for MnaGraph, MnaOpRegistry, and MnaGraphExecutor.
 *           Covers graph construction, validation, topological sorting,
 *           op registration, and batch / incremental execution.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mna/mna_graph.h>
#include <mne/base/log.h>
#include "../common/scoped_log_capture.h"
#include <mne/mna/mna_graph_executor.h>
#include <mne/mna/mna_op_registry.h>
#include <mne/mna/mna_node.h>
#include <mne/mna/mna_port.h>
#include <mne/mna/mna_types.h>
#include <mne/mna/mna_verification.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <any>
#include <memory>
#include <vector>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// HELPERS
//=============================================================================================================

namespace
{

MnaPort matrixPort(std::string name, MnaPortDir direction, MnaDataKind kind = MnaDataKind::Matrix)
{
    MnaPort port;
    port.name = std::move(name);
    port.data_kind = kind;
    port.direction = direction;
    return port;
}

/**
 * @brief Create a minimal node with one input port and one output port.
 */
MnaNode makeNode(const std::string& id, const std::string& opType)
{
    MnaNode node;
    node.id = id;
    node.op_type = opType;
    node.dirty = true;
    node.inputs.push_back(matrixPort("in", MnaPortDir::Input));
    node.outputs.push_back(matrixPort("out", MnaPortDir::Output));
    return node;
}

/**
 * @brief Source node — output only, no inputs.
 */
MnaNode makeSourceNode(const std::string& id, const std::string& opType)
{
    MnaNode node = makeNode(id, opType);
    node.inputs.clear();
    return node;
}

/**
 * @brief Sink node — input only, no outputs.
 */
MnaNode makeSinkNode(const std::string& id, const std::string& opType)
{
    MnaNode node = makeNode(id, opType);
    node.outputs.clear();
    return node;
}

/** @return The value of @p key in @p values as T; throws std::bad_any_cast if it holds another type. */
template<class T>
T value(const MnaValues& values, std::string_view key)
{
    return std::any_cast<T>(values.find(key)->second);
}

/** @return How many entries of @p list contain @p part. */
long countContaining(const StringList& list, std::string_view part)
{
    return std::ranges::count_if(list, [&](const std::string& s) { return s.find(part) != std::string::npos; });
}

/** @return The position of @p item in @p list. */
long position(const StringList& list, std::string_view item)
{
    return std::ranges::find(list, item) - list.begin();
}

/** A live stream plugin that records when it is released. */
struct Plugin
{
    Plugin(std::string opType, StringList& destroyed)
    : opType(std::move(opType))
    , destroyed(destroyed)
    {
    }
    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;
    ~Plugin()
    {
        destroyed.push_back(opType);
    }

    std::string opType;
    StringList& destroyed;
};

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestMnaGraphExecution
 *
 * @brief Tests for graph construction, validation, topology, op registry, and execution.
 */
class TestMnaGraphExecution : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- Graph construction ---
    void testEmptyGraph();
    void testAddRemoveNodes();
    void testHasNode();
    void testConnectNodes();

    // --- Validation ---
    void testValidateSimpleChain();
    void testValidateDisconnectedInput();
    void testValidateTypeMismatch();

    // --- Topology ---
    void testTopologicalSortLinear();
    void testTopologicalSortDiamond();
    void testDirtyNodes();
    void testUpstreamDownstream();

    // --- Op Registry ---
    void testRegistrySingleton();
    void testRegisterAndQuery();
    void testRegisteredOps();
    void testRegisterOpFunc();

    // --- Graph Executor ---
    void testExecuteLinearGraph();
    void testVerificationChecksAndProvenance();
    void testExecuteIncrementalCleanSkip();
    void testProgressCallback();
    void testStreamStartStop();
    void testScriptAndIpcNodes();
    void testScriptFileExtension_data();
    void testScriptFileExtension();

    void cleanupTestCase();

private:
    void registerTestOps();
};

//=============================================================================================================

void TestMnaGraphExecution::initTestCase()
{
    registerTestOps();
}

//=============================================================================================================

void TestMnaGraphExecution::registerTestOps()
{
    MnaOpRegistry& reg = MnaOpRegistry::instance();
    const auto registerOp = [&reg](const std::string& opType, const std::string& description, bool hasInput, bool hasOutput,
                                   MnaOpRegistry::OpFunc func) {
        if (reg.has_op(opType)) {
            return;
        }
        MnaOpSchema schema;
        schema.op_type = opType;
        schema.description = description;
        if (hasInput) {
            schema.input_ports.push_back({"in", MnaDataKind::Matrix, true, {}});
        }
        if (hasOutput) {
            schema.output_ports.push_back({"out", MnaDataKind::Matrix, true, {}});
        }
        reg.register_op(schema);
        reg.register_op_func(opType, std::move(func));
    };

    registerOp("test_source", "Emit a constant value", false, true, [](const MnaValues&, const JsonObject& attrs) -> MnaValues {
        return {{"out", attrs["value"].to_double(1.0)}};
    });
    registerOp("test_double", "Double the input value", true, true, [](const MnaValues& inputs, const JsonObject&) -> MnaValues {
        return {{"out", value<double>(inputs, "in") * 2.0}};
    });
    registerOp("test_add_one", "Add one to input", true, true, [](const MnaValues& inputs, const JsonObject&) -> MnaValues {
        return {{"out", value<double>(inputs, "in") + 1.0}};
    });
    registerOp("test_sink", "Consume input", true, false, [](const MnaValues&, const JsonObject&) -> MnaValues {
        return {};
    });
}

//=============================================================================================================
// Graph construction
//=============================================================================================================

void TestMnaGraphExecution::testEmptyGraph()
{
    MnaGraph graph;
    QVERIFY(graph.nodes().empty());
    // An empty graph is trivially valid (no broken connections)
    QVERIFY(graph.validate().empty());
}

//=============================================================================================================

void TestMnaGraphExecution::testAddRemoveNodes()
{
    MnaGraph graph;
    graph.add_node(makeNode("n1", "test_double"));
    graph.add_node(makeNode("n2", "test_add_one"));
    QCOMPARE(graph.nodes().size(), std::size_t(2));

    graph.remove_node("n1");
    graph.remove_node("missing");
    QCOMPARE(graph.nodes().size(), std::size_t(1));
    QVERIFY(!graph.find_node("missing"));
    QVERIFY(!std::as_const(graph).find_node("missing"));
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, graph.node("missing"));
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, std::as_const(graph).node("missing"));
    QVERIFY(!graph.has_node("n1"));
    QVERIFY(graph.has_node("n2"));
}

//=============================================================================================================

void TestMnaGraphExecution::testHasNode()
{
    MnaGraph graph;
    QVERIFY(!graph.has_node("x"));

    graph.add_node(makeNode("x", "test_double"));
    QVERIFY(graph.has_node("x"));
}

//=============================================================================================================

void TestMnaGraphExecution::testConnectNodes()
{
    MnaGraph graph;
    graph.add_node(makeSourceNode("src", "test_source"));
    graph.add_node(makeNode("mid", "test_double"));

    QVERIFY(graph.connect("src", "out", "mid", "in"));

    // Verify the input port picked up the connection
    const MnaNode& midNode = graph.node("mid");
    QCOMPARE(midNode.inputs.front().source_node_id, std::string("src"));
    QCOMPARE(midNode.inputs.front().source_port_name, std::string("out"));

    // Unknown nodes or ports are not connected
    QVERIFY(!graph.connect("nope", "out", "mid", "in"));
    QVERIFY(!graph.connect("src", "out", "nope", "in"));
    QVERIFY(!graph.connect("src", "nope", "mid", "in"));
    QVERIFY(!graph.connect("src", "out", "mid", "nope"));
}

//=============================================================================================================
// Validation
//=============================================================================================================

void TestMnaGraphExecution::testValidateSimpleChain()
{
    MnaGraph graph;
    graph.add_node(makeSourceNode("src", "test_source"));
    graph.add_node(makeNode("mid", "test_double"));
    graph.add_node(makeSinkNode("sink", "test_sink"));

    graph.connect("src", "out", "mid", "in");
    graph.connect("mid", "out", "sink", "in");

    const StringList errors = graph.validate();
    QVERIFY2(errors.empty(), join(errors, "; ").c_str());
}

//=============================================================================================================

void TestMnaGraphExecution::testValidateDisconnectedInput()
{
    MnaGraph graph;
    // Node with a required input but no upstream connection
    graph.add_node(makeNode("orphan", "test_double"));
    QVERIFY2(!graph.validate().empty(), "Expected validation failure for disconnected required input");

    // A cycle, a source that does not exist and an unregistered op are each reported
    MnaGraph broken;
    broken.add_node(makeNode("a", "test_double"));
    broken.add_node(makeNode("b", "test_double"));
    broken.add_node(makeNode("c", "no_such_op"));
    broken.connect("a", "out", "b", "in");
    broken.connect("b", "out", "a", "in");
    broken.node("c").inputs.front().source_node_id = "ghost";
    const StringList errors = broken.validate();
    QCOMPARE(countContaining(errors, "cycle"), 1);
    QCOMPARE(countContaining(errors, "ghost"), 1);
    QCOMPARE(countContaining(errors, "no_such_op"), 1);
    QCOMPARE(broken.topological_sort().size(), std::size_t(1));
}

//=============================================================================================================

void TestMnaGraphExecution::testValidateTypeMismatch()
{
    MnaGraph graph;

    // Source with Epochs output feeding a consumer that expects a Matrix input
    MnaNode src = makeSourceNode("src", "test_source");
    src.outputs.front().data_kind = MnaDataKind::Epochs;
    graph.add_node(src);
    graph.add_node(makeNode("consumer", "test_double"));
    graph.connect("src", "out", "consumer", "in");

    QVERIFY2(countContaining(graph.validate(), "data kind mismatch") == 1, "Expected validation failure for data kind mismatch");
}

//=============================================================================================================
// Topology
//=============================================================================================================

void TestMnaGraphExecution::testTopologicalSortLinear()
{
    MnaGraph graph;
    graph.add_node(makeSourceNode("A", "test_source"));
    graph.add_node(makeNode("B", "test_double"));
    graph.add_node(makeSinkNode("C", "test_sink"));

    graph.connect("A", "out", "B", "in");
    graph.connect("B", "out", "C", "in");

    QCOMPARE(graph.topological_sort(), (StringList{"A", "B", "C"}));
}

//=============================================================================================================

void TestMnaGraphExecution::testTopologicalSortDiamond()
{
    // A → B, A → C, B → D
    MnaGraph graph;
    graph.add_node(makeSourceNode("A", "test_source"));
    graph.add_node(makeNode("B", "test_double"));
    graph.add_node(makeNode("C", "test_add_one"));
    graph.add_node(makeSinkNode("D", "test_sink"));

    graph.connect("A", "out", "B", "in");
    graph.connect("A", "out", "C", "in");
    graph.connect("B", "out", "D", "in");

    const StringList order = graph.topological_sort();
    QCOMPARE(order.size(), std::size_t(4));
    QVERIFY(position(order, "A") < position(order, "B"));
    QVERIFY(position(order, "A") < position(order, "C"));
    QVERIFY(position(order, "B") < position(order, "D"));
}

//=============================================================================================================

void TestMnaGraphExecution::testDirtyNodes()
{
    MnaGraph graph;

    MnaNode clean = makeSourceNode("clean", "test_source");
    clean.dirty = false;
    graph.add_node(clean);
    graph.add_node(makeNode("dirty", "test_double"));

    QCOMPARE(graph.dirty_nodes(), StringList{"dirty"});
}

//=============================================================================================================

void TestMnaGraphExecution::testUpstreamDownstream()
{
    MnaGraph graph;
    graph.add_node(makeSourceNode("A", "test_source"));
    graph.add_node(makeNode("B", "test_double"));
    graph.add_node(makeSinkNode("C", "test_sink"));

    graph.connect("A", "out", "B", "in");
    graph.connect("B", "out", "C", "in");

    QCOMPARE(graph.upstream_nodes("C"), (StringList{"B", "A"}));
    QCOMPARE(graph.downstream_nodes("A"), (StringList{"B", "C"}));
    QVERIFY(graph.upstream_nodes("A").empty());
    QVERIFY(graph.upstream_nodes("nope").empty());
    QVERIFY(graph.downstream_nodes("nope").empty());
    QVERIFY(graph.downstream_nodes("C").empty());
}

//=============================================================================================================
// Op Registry
//=============================================================================================================

void TestMnaGraphExecution::testRegistrySingleton()
{
    MnaOpRegistry& a = MnaOpRegistry::instance();
    MnaOpRegistry& b = MnaOpRegistry::instance();
    QCOMPARE(&a, &b);
}

//=============================================================================================================

void TestMnaGraphExecution::testRegisterAndQuery()
{
    MnaOpRegistry& reg = MnaOpRegistry::instance();
    QVERIFY(reg.has_op("test_double"));

    const MnaOpSchema& schema = reg.schema("test_double");
    QCOMPARE(schema.op_type, std::string("test_double"));
    QCOMPARE(schema.input_ports.size(), std::size_t(1));
    QCOMPARE(schema.output_ports.size(), std::size_t(1));
    QVERIFY(reg.schema("no_such_op").op_type.empty());
}

//=============================================================================================================

void TestMnaGraphExecution::testRegisteredOps()
{
    const StringList ops = MnaOpRegistry::instance().registered_ops();
    for (const char* op : {"test_source", "test_double", "test_add_one", "test_sink"}) {
        QVERIFY2(std::ranges::contains(ops, std::string_view(op)), op);
    }
}

//=============================================================================================================

void TestMnaGraphExecution::testRegisterOpFunc()
{
    MnaOpRegistry& reg = MnaOpRegistry::instance();

    const MnaOpRegistry::OpFunc& func = reg.op_func("test_double");
    QVERIFY(func);
    QVERIFY(!reg.op_func("no_such_op"));

    const MnaValues result = func({{"in", 5.0}}, {});
    QCOMPARE(value<double>(result, "out"), 10.0);
}

//=============================================================================================================
// Graph Executor
//=============================================================================================================

void TestMnaGraphExecution::testStreamStartStop()
{
    MnaGraph graph;
    graph.add_node(makeSourceNode("src", "test_source"));
    graph.add_node(makeNode("dbl", "test_double"));
    graph.connect("src", "out", "dbl", "in");
    graph.param_tree.set_param("src/value", 5.0);
    graph.param_tree.set_param("missing/value", 1.0);

    // One live plugin per node, created in topological order and released in reverse
    StringList created;
    StringList destroyed;
    const auto factory = [&](std::string_view opType) -> std::shared_ptr<void> {
        created.emplace_back(opType);
        return std::make_shared<Plugin>(std::string(opType), destroyed);
    };
    MnaGraphExecutor::StreamContext ctx = MnaGraphExecutor::start_stream(graph, factory);
    QVERIFY(ctx.running);
    QCOMPARE(ctx.graph, &graph);
    QCOMPARE(ctx.execution_order, (StringList{"src", "dbl"}));
    QCOMPARE(created, (StringList{"test_source", "test_double"}));
    QCOMPARE(std::static_pointer_cast<Plugin>(ctx.live_plugins.at("dbl"))->opType, std::string("test_double"));
    QCOMPARE(graph.node("src").attributes["value"].to_double(), 5.0);
    MnaGraphExecutor::stop_stream(ctx);
    QVERIFY(!ctx.running && ctx.live_plugins.empty() && !ctx.graph);
    QCOMPARE(destroyed, (StringList{"test_double", "test_source"}));
    MnaGraphExecutor::stop_stream(ctx);

    // A plugin that cannot be created aborts the start and frees those already created
    destroyed.clear();
    const auto partial = [&](std::string_view opType) -> std::shared_ptr<void> {
        return opType == "test_double" ? nullptr : factory(opType);
    };
    ScopedLogCapture capture;
    ctx = MnaGraphExecutor::start_stream(graph, partial);
    QVERIFY2(capture.contains(mne::Log::Level::Warning, "factory returned nullptr"), capture.dump().c_str());
    QVERIFY(!ctx.running && ctx.live_plugins.empty());
    QCOMPARE(destroyed, StringList{"test_source"});

    // An invalid graph (unconnected input) does not start
    MnaGraph invalid;
    invalid.add_node(makeNode("dbl", "test_double"));
    capture.clear();
    QVERIFY(!MnaGraphExecutor::start_stream(invalid, factory).running);
    QVERIFY2(capture.contains(mne::Log::Level::Warning, "graph validation failed"), capture.dump().c_str());
}

//=============================================================================================================

void TestMnaGraphExecution::testScriptAndIpcNodes()
{
#ifdef Q_OS_WIN
    QSKIP("Uses /bin/sh and /bin/echo");
#else
    // Script: {{placeholders}} take input values and attributes; stdout, stderr and the exit code become outputs
    MnaGraph graph;
    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 3.0;
    graph.add_node(src);
    MnaNode script = makeNode("script", "shell_script");
    script.exec_mode = MnaNodeExecMode::Script;
    script.script.language = "shell";
    script.script.interpreter = "/bin/sh";
    script.script.code = "echo in={{in}} label={{label}}; echo oops >&2; exit 3";
    script.attributes["label"] = "alpha";
    graph.add_node(script);
    graph.connect("src", "out", "script", "in");

    // IPC: an external command with resolved arguments, run in the given directory; cached result paths are passed on
    MnaNode ipc = makeNode("ipc", "external");
    ipc.exec_mode = MnaNodeExecMode::Ipc;
    ipc.ipc_command = "/bin/sh";
    ipc.ipc_args = {"-c", "echo {{in}} $(pwd)"};
    QTemporaryDir workDir;
    ipc.ipc_work_dir = QDir(workDir.path()).canonicalPath().toStdString();
    ipc.outputs[0].cached_result_path = "result.fif";
    graph.add_node(ipc);
    graph.connect("src", "out", "ipc", "in");

    const MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph);
    QCOMPARE(value<std::string>(ctx.results, "script::stdout"), std::string("in=3 label=alpha\n"));
    QCOMPARE(value<std::string>(ctx.results, "script::stderr"), std::string("oops\n"));
    QCOMPARE(value<int>(ctx.results, "script::exit_code"), 3);
    QCOMPARE(value<std::string>(ctx.results, "ipc::stdout"), "3 " + ipc.ipc_work_dir + "\n");
    QCOMPARE(value<int>(ctx.results, "ipc::exit_code"), 0);
    QCOMPARE(value<std::string>(ctx.results, "ipc::out"), std::string("result.fif"));

    // A command that cannot start is a failure, not exit code 0
    MnaGraph missingGraph;
    MnaNode missing = makeNode("missing", "external");
    missing.exec_mode = MnaNodeExecMode::Ipc;
    missing.ipc_command = "mne-cpp-no-such-program";
    missingGraph.add_node(missing);
    const MnaGraphExecutor::Context missingCtx = MnaGraphExecutor::execute(missingGraph);
    QCOMPARE(value<int>(missingCtx.results, "missing::exit_code"), -1);
    QVERIFY(!value<std::string>(missingCtx.results, "missing::stderr").empty());
#endif
}

//=============================================================================================================

void TestMnaGraphExecution::testScriptFileExtension_data()
{
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("interpreter");
    QTest::addColumn<QString>("extension");

    QTest::newRow("python") << "python" << "/bin/sh" << "py";
    QTest::newRow("shell") << "shell" << "/bin/sh" << "sh";
    QTest::newRow("shell-default-bash") << "shell" << "" << "sh";
    QTest::newRow("r") << "r" << "/bin/sh" << "R";
    QTest::newRow("matlab") << "matlab" << "/bin/sh" << "m";
    QTest::newRow("octave") << "octave" << "/bin/sh" << "m";
    QTest::newRow("julia") << "julia" << "/bin/sh" << "jl";
    QTest::newRow("other") << "perl" << "/bin/sh" << "txt";
}

void TestMnaGraphExecution::testScriptFileExtension()
{
#ifdef Q_OS_WIN
    QSKIP("Uses /bin/sh and /bin/bash");
#else
    QFETCH(QString, language);
    QFETCH(QString, interpreter);
    QFETCH(QString, extension);

    // The interpreter receives the temporary script file; the script prints that file's extension.
    MnaGraph graph;
    MnaNode script = makeSourceNode("script", "script");
    script.exec_mode = MnaNodeExecMode::Script;
    script.script.language = language.toStdString();
    script.script.interpreter = interpreter.toStdString();
    script.script.code = "printf %s \"${0##*.}\"";
    graph.add_node(script);

    const MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph);
    QCOMPARE(value<int>(ctx.results, "script::exit_code"), 0);
    QCOMPARE(value<std::string>(ctx.results, "script::stdout"), extension.toStdString());
#endif
}

//=============================================================================================================

void TestMnaGraphExecution::testExecuteLinearGraph()
{
    // Chain: source(value=3) → double → add_one; expected 3 → 6 → 7
    MnaGraph graph;
    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 3.0;
    graph.add_node(src);
    graph.add_node(makeNode("dbl", "test_double"));
    graph.add_node(makeNode("add", "test_add_one"));
    graph.connect("src", "out", "dbl", "in");
    graph.connect("dbl", "out", "add", "in");

    const MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph);
    QCOMPARE(value<double>(ctx.results, "src::out"), 3.0);
    QCOMPARE(value<double>(ctx.results, "dbl::out"), 6.0);
    QCOMPARE(value<double>(ctx.results, "add::out"), 7.0);
}

//=============================================================================================================

void TestMnaGraphExecution::testVerificationChecksAndProvenance()
{
    // source(value=3) -> double -> add_one, with checks on "dbl"
    MnaGraph graph;
    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 3.0;
    graph.add_node(src);
    MnaNode dbl = makeNode("dbl", "test_double");
    dbl.attributes["limit"] = 10.0;
    const MnaVerificationCheck positive{"in_positive", "Input is positive", "pre", "in > 0", {}, "error", {}};
    const MnaVerificationCheck underLimit{"out_under_limit", "Output stays below the limit", "post", "out < ref('limit')", {}, "error", "lower the source value"};
    const MnaVerificationCheck exact{"out_is_six", "Output is six", "post", "out == 6", {}, "info", {}};
    dbl.verification.checks = {positive, underLimit, exact};
    graph.add_node(dbl);
    graph.add_node(makeNode("add", "test_add_one"));
    graph.connect("src", "out", "dbl", "in");
    graph.connect("dbl", "out", "add", "in");

    MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph);
    const MnaVerification& v = graph.node("dbl").verification;
    QVERIFY(ctx.aborted_node.empty());
    QCOMPARE(value<double>(ctx.results, "add::out"), 7.0);
    QCOMPARE(v.pre_results.size(), std::size_t(1));
    QCOMPARE(v.post_results.size(), std::size_t(2));
    QVERIFY(v.pre_results[0].passed && v.post_results[0].passed && v.post_results[1].passed);
    QCOMPARE(v.post_results[0].message, std::string("PASS: Output stays below the limit"));
    QCOMPARE(v.provenance.mne_cpp_version, std::string(MNE_CPP_VERSION));
    QCOMPARE(v.provenance.resolved_attributes["limit"].to_double(), 10.0);
    // SHA-256 of the input's text "3"
    QCOMPARE(v.provenance.input_hashes.at("in"), std::string("4e07408562bedb8b60ce05c1decfe3ad16b72230967de01f640b7e4729b49fce"));
    QVERIFY(v.provenance.started_at && v.provenance.finished_at && *v.provenance.finished_at >= *v.provenance.started_at);

    // Comparison operators of check expressions, at their boundaries
    const JsonObject at6{{"out", 6.0}};
    for (const auto& [expr, expected] : {std::pair{"out >= 6", true}, {"out <= 6", true}, {"out > 6", false}, {"out < 6", false}, {"out != 6", false}, {"out == 6.0", true}}) {
        QCOMPARE(graph.param_tree.evaluate_expression(expr, at6).to_bool(), expected);
    }

    // An "error" post check that fails stops the run before "add"
    graph.node("src").attributes["value"] = 6.0;
    graph.node("src").dirty = true;
    MnaGraphExecutor::execute_incremental(graph, ctx);
    QCOMPARE(ctx.aborted_node, std::string("dbl"));
    QCOMPARE(value<double>(ctx.results, "add::out"), 7.0);
    QVERIFY(!graph.node("dbl").verification.post_results[0].passed);
    QCOMPARE(graph.node("dbl").verification.post_results[0].message, std::string("FAIL [error]: Output stays below the limit (lower the source value)"));
    QVERIFY(!graph.node("dbl").verification.post_results[1].passed); // "info" fails but does not stop

    // A failing "error" pre check skips the node itself
    graph.node("src").attributes["value"] = -1.0;
    graph.node("src").dirty = true;
    ctx.aborted_node.clear();
    MnaGraphExecutor::execute_incremental(graph, ctx);
    QCOMPARE(ctx.aborted_node, std::string("dbl"));
    QVERIFY(!graph.node("dbl").verification.pre_results[0].passed);
    QVERIFY(graph.node("dbl").verification.post_results.empty());
    QCOMPARE(value<double>(ctx.results, "dbl::out"), 12.0);
}

//=============================================================================================================

void TestMnaGraphExecution::testExecuteIncrementalCleanSkip()
{
    // Execute once, mark all clean, then an incremental run must not re-execute
    MnaGraph graph;
    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 10.0;
    graph.add_node(src);
    graph.add_node(makeNode("dbl", "test_double"));
    graph.connect("src", "out", "dbl", "in");

    MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph);
    QCOMPARE(value<double>(ctx.results, "dbl::out"), 20.0);
    for (const MnaNode& n : graph.nodes()) {
        QVERIFY(!n.dirty && n.executed_at); // execution resets the dirty flag
    }

    // Incremental — no dirty nodes, results remain from the previous run
    graph.node("src").attributes["value"] = 99.0;
    MnaGraphExecutor::execute_incremental(graph, ctx);
    QCOMPARE(value<double>(ctx.results, "dbl::out"), 20.0);

    // Parameter-tree values reach the nodes in incremental runs as well
    graph.param_tree.set_param("src/value", 4.0);
    graph.node("src").dirty = true;
    MnaGraphExecutor::execute_incremental(graph, ctx);
    QCOMPARE(value<double>(ctx.results, "dbl::out"), 8.0);
}

//=============================================================================================================

void TestMnaGraphExecution::testProgressCallback()
{
    MnaGraph graph;
    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 1.0;
    graph.add_node(src);
    graph.add_node(makeNode("dbl", "test_double"));
    graph.connect("src", "out", "dbl", "in");

    StringList visited;
    std::vector<std::pair<int, int>> steps;
    MnaGraphExecutor::set_progress_callback([&](std::string_view nodeId, int current, int total) {
        visited.emplace_back(nodeId);
        steps.emplace_back(current, total);
    });
    MnaGraphExecutor::execute(graph);
    MnaGraphExecutor::set_progress_callback(nullptr);
    MnaGraphExecutor::execute(graph);

    // Both nodes were visited once, in topological order
    QCOMPARE(visited, (StringList{"src", "dbl"}));
    QVERIFY((steps == std::vector<std::pair<int, int>>{{1, 2}, {2, 2}}));
}

//=============================================================================================================

void TestMnaGraphExecution::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaGraphExecution)
#include "test_mna_graph_execution.moc"
