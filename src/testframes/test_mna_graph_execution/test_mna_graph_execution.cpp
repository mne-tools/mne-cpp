//=============================================================================================================
/**
 * @file     test_mna_graph_execution.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Tests for MnaGraph, MnaOpRegistry, and MnaGraphExecutor.
 *           Covers graph construction, validation, topological sorting,
 *           op registration, and batch / incremental execution.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_graph.h>
#include <mna/mna_graph_executor.h>
#include <mna/mna_op_registry.h>
#include <mna/mna_node.h>
#include <mna/mna_port.h>
#include <mna/mna_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QVariant>
#include <QVariantMap>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// HELPERS
//=============================================================================================================

namespace {

/**
 * @brief Create a minimal node with one input port and one output port.
 */
MnaNode makeNode(const QString& id, const QString& opType)
{
    MnaNode node;
    node.id     = id;
    node.opType = opType;
    node.dirty  = true;

    MnaPort in;
    in.name      = "in";
    in.dataKind  = MnaDataKind::Matrix;
    in.direction = MnaPortDir::Input;
    node.inputs.append(in);

    MnaPort out;
    out.name      = "out";
    out.dataKind  = MnaDataKind::Matrix;
    out.direction = MnaPortDir::Output;
    node.outputs.append(out);

    return node;
}

/**
 * @brief Source node — output only, no inputs.
 */
MnaNode makeSourceNode(const QString& id, const QString& opType)
{
    MnaNode node;
    node.id     = id;
    node.opType = opType;
    node.dirty  = true;

    MnaPort out;
    out.name      = "out";
    out.dataKind  = MnaDataKind::Matrix;
    out.direction = MnaPortDir::Output;
    node.outputs.append(out);

    return node;
}

/**
 * @brief Sink node — input only, no outputs.
 */
MnaNode makeSinkNode(const QString& id, const QString& opType)
{
    MnaNode node;
    node.id     = id;
    node.opType = opType;
    node.dirty  = true;

    MnaPort in;
    in.name      = "in";
    in.dataKind  = MnaDataKind::Matrix;
    in.direction = MnaPortDir::Input;
    node.inputs.append(in);

    return node;
}

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
    void testExecuteIncrementalCleanSkip();
    void testProgressCallback();

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

    // "test_source" — generates a constant matrix value
    if (!reg.hasOp("test_source")) {
        MnaOpSchema sourceSchema;
        sourceSchema.opType  = "test_source";
        sourceSchema.description = "Emit a constant value";

        MnaOpSchemaPort outPort;
        outPort.name     = "out";
        outPort.dataKind = MnaDataKind::Matrix;
        sourceSchema.outputPorts.append(outPort);

        reg.registerOp(sourceSchema);
        reg.registerOpFunc("test_source", [](const QVariantMap& /*inputs*/,
                                              const QVariantMap& attrs) -> QVariantMap {
            double value = attrs.value("value", 1.0).toDouble();
            return {{"out", value}};
        });
    }

    // "test_double" — doubles the input
    if (!reg.hasOp("test_double")) {
        MnaOpSchema dblSchema;
        dblSchema.opType  = "test_double";
        dblSchema.description = "Double the input value";

        MnaOpSchemaPort inPort;
        inPort.name     = "in";
        inPort.dataKind = MnaDataKind::Matrix;
        inPort.required = true;
        dblSchema.inputPorts.append(inPort);

        MnaOpSchemaPort outPort;
        outPort.name     = "out";
        outPort.dataKind = MnaDataKind::Matrix;
        dblSchema.outputPorts.append(outPort);

        reg.registerOp(dblSchema);
        reg.registerOpFunc("test_double", [](const QVariantMap& inputs,
                                              const QVariantMap& /*attrs*/) -> QVariantMap {
            double val = inputs.value("in", 0.0).toDouble();
            return {{"out", val * 2.0}};
        });
    }

    // "test_add_one" — adds 1 to the input
    if (!reg.hasOp("test_add_one")) {
        MnaOpSchema addSchema;
        addSchema.opType  = "test_add_one";
        addSchema.description = "Add one to input";

        MnaOpSchemaPort inPort;
        inPort.name     = "in";
        inPort.dataKind = MnaDataKind::Matrix;
        inPort.required = true;
        addSchema.inputPorts.append(inPort);

        MnaOpSchemaPort outPort;
        outPort.name     = "out";
        outPort.dataKind = MnaDataKind::Matrix;
        addSchema.outputPorts.append(outPort);

        reg.registerOp(addSchema);
        reg.registerOpFunc("test_add_one", [](const QVariantMap& inputs,
                                               const QVariantMap& /*attrs*/) -> QVariantMap {
            double val = inputs.value("in", 0.0).toDouble();
            return {{"out", val + 1.0}};
        });
    }

    // "test_sink" — consumes input, produces nothing
    if (!reg.hasOp("test_sink")) {
        MnaOpSchema sinkSchema;
        sinkSchema.opType  = "test_sink";
        sinkSchema.description = "Consume input";

        MnaOpSchemaPort inPort;
        inPort.name     = "in";
        inPort.dataKind = MnaDataKind::Matrix;
        inPort.required = true;
        sinkSchema.inputPorts.append(inPort);

        reg.registerOp(sinkSchema);
        reg.registerOpFunc("test_sink", [](const QVariantMap& inputs,
                                            const QVariantMap& /*attrs*/) -> QVariantMap {
            Q_UNUSED(inputs);
            return {};
        });
    }
}

//=============================================================================================================
// Graph construction
//=============================================================================================================

void TestMnaGraphExecution::testEmptyGraph()
{
    MnaGraph graph;
    QVERIFY(graph.nodes().isEmpty());

    QStringList errors;
    // An empty graph is trivially valid (no broken connections)
    bool valid = graph.validate(&errors);
    QVERIFY(valid);
}

//=============================================================================================================

void TestMnaGraphExecution::testAddRemoveNodes()
{
    MnaGraph graph;
    MnaNode n1 = makeNode("n1", "test_double");
    MnaNode n2 = makeNode("n2", "test_add_one");

    graph.addNode(n1);
    graph.addNode(n2);
    QCOMPARE(graph.nodes().size(), 2);

    graph.removeNode("n1");
    QCOMPARE(graph.nodes().size(), 1);
    QVERIFY(!graph.hasNode("n1"));
    QVERIFY(graph.hasNode("n2"));
}

//=============================================================================================================

void TestMnaGraphExecution::testHasNode()
{
    MnaGraph graph;
    QVERIFY(!graph.hasNode("x"));

    graph.addNode(makeNode("x", "test_double"));
    QVERIFY(graph.hasNode("x"));
}

//=============================================================================================================

void TestMnaGraphExecution::testConnectNodes()
{
    MnaGraph graph;
    graph.addNode(makeSourceNode("src", "test_source"));
    graph.addNode(makeNode("mid", "test_double"));

    bool ok = graph.connect("src", "out", "mid", "in");
    QVERIFY(ok);

    // Verify the input port picked up the connection
    const MnaNode& midNode = graph.node("mid");
    QCOMPARE(midNode.inputs.first().sourceNodeId, QString("src"));
    QCOMPARE(midNode.inputs.first().sourcePortName, QString("out"));
}

//=============================================================================================================
// Validation
//=============================================================================================================

void TestMnaGraphExecution::testValidateSimpleChain()
{
    MnaGraph graph;
    graph.addNode(makeSourceNode("src", "test_source"));
    graph.addNode(makeNode("mid", "test_double"));
    graph.addNode(makeSinkNode("sink", "test_sink"));

    graph.connect("src", "out", "mid", "in");
    graph.connect("mid", "out", "sink", "in");

    QStringList errors;
    QVERIFY2(graph.validate(&errors),
             qPrintable(errors.join("; ")));
}

//=============================================================================================================

void TestMnaGraphExecution::testValidateDisconnectedInput()
{
    MnaGraph graph;
    // Node with a required input but no upstream connection
    graph.addNode(makeNode("orphan", "test_double"));

    QStringList errors;
    bool valid = graph.validate(&errors);
    // A disconnected required port should cause validation to fail
    QVERIFY2(!valid, "Expected validation failure for disconnected required input");
    QVERIFY(!errors.isEmpty());
}

//=============================================================================================================

void TestMnaGraphExecution::testValidateTypeMismatch()
{
    MnaGraph graph;

    // Create source with Epochs output
    MnaNode src;
    src.id     = "src";
    src.opType = "test_source";
    {
        MnaPort out;
        out.name      = "out";
        out.dataKind  = MnaDataKind::Epochs;
        out.direction = MnaPortDir::Output;
        src.outputs.append(out);
    }

    // Create consumer expecting Matrix input
    MnaNode consumer = makeNode("consumer", "test_double");

    graph.addNode(src);
    graph.addNode(consumer);
    graph.connect("src", "out", "consumer", "in");

    QStringList errors;
    bool valid = graph.validate(&errors);
    // Should detect MnaDataKind mismatch
    QVERIFY2(!valid, "Expected validation failure for data kind mismatch");
}

//=============================================================================================================
// Topology
//=============================================================================================================

void TestMnaGraphExecution::testTopologicalSortLinear()
{
    MnaGraph graph;
    graph.addNode(makeSourceNode("A", "test_source"));
    graph.addNode(makeNode("B", "test_double"));
    graph.addNode(makeSinkNode("C", "test_sink"));

    graph.connect("A", "out", "B", "in");
    graph.connect("B", "out", "C", "in");

    QStringList order = graph.topologicalSort();
    QCOMPARE(order.size(), 3);

    // A must come before B, B before C
    QVERIFY(order.indexOf("A") < order.indexOf("B"));
    QVERIFY(order.indexOf("B") < order.indexOf("C"));
}

//=============================================================================================================

void TestMnaGraphExecution::testTopologicalSortDiamond()
{
    // A → B, A → C, B → D, C → D  (diamond)
    MnaGraph graph;
    graph.addNode(makeSourceNode("A", "test_source"));
    graph.addNode(makeNode("B", "test_double"));
    graph.addNode(makeNode("C", "test_add_one"));
    graph.addNode(makeSinkNode("D", "test_sink"));

    graph.connect("A", "out", "B", "in");
    graph.connect("A", "out", "C", "in");
    graph.connect("B", "out", "D", "in");

    QStringList order = graph.topologicalSort();
    QCOMPARE(order.size(), 4);
    QVERIFY(order.indexOf("A") < order.indexOf("B"));
    QVERIFY(order.indexOf("A") < order.indexOf("C"));
    QVERIFY(order.indexOf("B") < order.indexOf("D"));
}

//=============================================================================================================

void TestMnaGraphExecution::testDirtyNodes()
{
    MnaGraph graph;

    MnaNode clean = makeSourceNode("clean", "test_source");
    clean.dirty = false;
    MnaNode dirty = makeNode("dirty", "test_double");
    dirty.dirty = true;

    graph.addNode(clean);
    graph.addNode(dirty);

    QStringList dirtyList = graph.dirtyNodes();
    QVERIFY(dirtyList.contains("dirty"));
    QVERIFY(!dirtyList.contains("clean"));
}

//=============================================================================================================

void TestMnaGraphExecution::testUpstreamDownstream()
{
    MnaGraph graph;
    graph.addNode(makeSourceNode("A", "test_source"));
    graph.addNode(makeNode("B", "test_double"));
    graph.addNode(makeSinkNode("C", "test_sink"));

    graph.connect("A", "out", "B", "in");
    graph.connect("B", "out", "C", "in");

    QStringList upstream = graph.upstreamNodes("C");
    QVERIFY(upstream.contains("B"));

    QStringList downstream = graph.downstreamNodes("A");
    QVERIFY(downstream.contains("B"));
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
    QVERIFY(reg.hasOp("test_double"));

    MnaOpSchema schema = reg.schema("test_double");
    QCOMPARE(schema.opType, QString("test_double"));
    QCOMPARE(schema.inputPorts.size(), 1);
    QCOMPARE(schema.outputPorts.size(), 1);
}

//=============================================================================================================

void TestMnaGraphExecution::testRegisteredOps()
{
    MnaOpRegistry& reg = MnaOpRegistry::instance();
    QStringList ops = reg.registeredOps();

    QVERIFY(ops.contains("test_source"));
    QVERIFY(ops.contains("test_double"));
    QVERIFY(ops.contains("test_add_one"));
    QVERIFY(ops.contains("test_sink"));
}

//=============================================================================================================

void TestMnaGraphExecution::testRegisterOpFunc()
{
    MnaOpRegistry& reg = MnaOpRegistry::instance();

    auto func = reg.opFunc("test_double");
    QVERIFY(func != nullptr);

    QVariantMap result = func({{"in", 5.0}}, {});
    QCOMPARE(result.value("out").toDouble(), 10.0);
}

//=============================================================================================================
// Graph Executor
//=============================================================================================================

void TestMnaGraphExecution::testExecuteLinearGraph()
{
    // Chain: source(value=3) → double → add_one
    // Expected: 3 → 6 → 7
    MnaGraph graph;

    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 3.0;
    graph.addNode(src);

    graph.addNode(makeNode("dbl", "test_double"));
    graph.addNode(makeNode("add", "test_add_one"));

    graph.connect("src", "out", "dbl", "in");
    graph.connect("dbl", "out", "add", "in");

    MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph, {});

    // Check intermediate: src output = 3
    double srcOut = ctx.results.value("src::out").toDouble();
    QCOMPARE(srcOut, 3.0);

    // dbl output = 6
    double dblOut = ctx.results.value("dbl::out").toDouble();
    QCOMPARE(dblOut, 6.0);

    // add output = 7
    double addOut = ctx.results.value("add::out").toDouble();
    QCOMPARE(addOut, 7.0);
}

//=============================================================================================================

void TestMnaGraphExecution::testExecuteIncrementalCleanSkip()
{
    // Build same chain, execute once, mark all clean, then incremental should not re-execute
    MnaGraph graph;

    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 10.0;
    graph.addNode(src);
    graph.addNode(makeNode("dbl", "test_double"));

    graph.connect("src", "out", "dbl", "in");

    // Full execution
    MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph, {});
    double dblOut = ctx.results.value("dbl::out").toDouble();
    QCOMPARE(dblOut, 20.0);

    // Mark nodes clean (execution resets dirty flag)
    for (MnaNode& n : graph.nodes()) {
        n.dirty = false;
    }

    // Incremental — no dirty nodes, results should remain from previous context
    MnaGraphExecutor::Context ctx2 = MnaGraphExecutor::executeIncremental(graph, ctx);
    double dblOut2 = ctx2.results.value("dbl::out").toDouble();
    QCOMPARE(dblOut2, 20.0);
}

//=============================================================================================================

void TestMnaGraphExecution::testProgressCallback()
{
    MnaGraph graph;

    MnaNode src = makeSourceNode("src", "test_source");
    src.attributes["value"] = 1.0;
    graph.addNode(src);
    graph.addNode(makeNode("dbl", "test_double"));

    graph.connect("src", "out", "dbl", "in");

    QStringList visited;
    MnaGraphExecutor::setProgressCallback(
        [&visited](const QString& nodeId, int /*current*/, int /*total*/) {
            visited.append(nodeId);
        });

    MnaGraphExecutor::execute(graph, {});

    // Reset callback
    MnaGraphExecutor::setProgressCallback(nullptr);

    // Both nodes should have been visited in topological order
    QVERIFY(visited.contains("src"));
    QVERIFY(visited.contains("dbl"));
    QVERIFY(visited.indexOf("src") < visited.indexOf("dbl"));
}

//=============================================================================================================

void TestMnaGraphExecution::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaGraphExecution)
#include "test_mna_graph_execution.moc"
