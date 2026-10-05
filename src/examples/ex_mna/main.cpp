//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    MNA library: register operations, build and run a graph, bind parameters and save a project.
 *
 * Two operations are declared in a registry file and given C++ implementations;
 * a three-node graph is validated, executed (also incrementally) with a
 * parameter-tree binding, and stored as .mna (JSON) and .mnx (CBOR) projects.
 * Every result is a closed form. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_graph.h>
#include <mna/mna_graph_executor.h>
#include <mna/mna_io.h>
#include <mna/mna_op_registry.h>
#include <mna/mna_op_schema.h>
#include <mna/mna_param_tree.h>
#include <mna/mna_project.h>
#include <mna/mna_registry_loader.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

MnaPort port(const QString& name, MnaPortDir direction)
{
    MnaPort p;
    p.name = name;
    p.dataKind = MnaDataKind::Matrix;
    p.direction = direction;
    return p;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    bool ok = dir.isValid();

    QFile registryFile(dir.filePath("ex-registry.json"));
    ok &= registryFile.open(QIODevice::WriteOnly) && registryFile.write(R"({
  "mna_registry_version": "1.0", "provider": "ex_mna",
  "ops": [
    {"type": "ex_constant", "binding": "internal", "category": "io", "description": "Emit a constant",
     "outputs": [{"name": "out", "kind": "Matrix"}],
     "parameters": [{"name": "value", "type": "double", "required": true}]},
    {"type": "ex_affine", "binding": "internal", "category": "preprocessing", "description": "gain * in + offset",
     "inputs": [{"name": "in", "kind": "Matrix"}], "outputs": [{"name": "out", "kind": "Matrix"}],
     "parameters": [{"name": "gain", "type": "double", "default": 1.0}, {"name": "offset", "type": "double", "default": 0.0}]}
  ]
})") > 0;
    registryFile.close();

    //! [mna_registry_loader_usage]
    MnaOpRegistry& registry = MnaOpRegistry::instance();
    const int nLoaded = MnaRegistryLoader::loadFile(registryFile.fileName(), registry); // schemas only
    registry.registerOpFunc("ex_constant", [](const QVariantMap&, const QVariantMap& attrs) -> QVariantMap {
        return {{"out", attrs.value("value").toDouble()}};
    });
    registry.registerOpFunc("ex_affine", [](const QVariantMap& inputs, const QVariantMap& attrs) -> QVariantMap {
        return {{"out", attrs.value("gain", 1.0).toDouble() * inputs.value("in").toDouble() + attrs.value("offset", 0.0).toDouble()}};
    });
    //! [mna_registry_loader_usage]
    ok &= expect(nLoaded == 2 && registry.hasOp("ex_affine") && registry.schema("ex_affine").inputPorts.size() == 1 && registry.missingOps({"ex_constant", "ex_affine", "ex_unknown"}) == QStringList({"ex_unknown"}),
                 "MnaRegistryLoader loads 2 ops; missingOps reports the unknown one");

    //! [mna_graph_build]
    MnaGraph graph;
    MnaNode source;
    source.id = "source";
    source.opType = "ex_constant";
    source.attributes["value"] = 3.0;
    source.outputs.append(port("out", MnaPortDir::Output));
    graph.addNode(source);

    for (const QString& id : {QStringLiteral("scale"), QStringLiteral("shift")}) {
        MnaNode node;
        node.id = id;
        node.opType = "ex_affine";
        node.inputs.append(port("in", MnaPortDir::Input));
        node.outputs.append(port("out", MnaPortDir::Output));
        graph.addNode(node);
    }
    graph.node("scale").attributes["gain"] = 2.0;
    graph.connect("source", "out", "scale", "in");
    graph.connect("scale", "out", "shift", "in");
    //! [mna_graph_build]

    //! [mna_op_schema_validate]
    QStringList problems;
    const bool nodeValid = registry.schema("ex_affine").validate(graph.node("scale"), &problems);
    MnaNode unset = graph.node("source");
    unset.attributes.clear(); // "value" is required
    const bool unsetValid = registry.schema("ex_constant").validate(unset);
    //! [mna_op_schema_validate]
    ok &= expect(nodeValid && problems.isEmpty() && !unsetValid && graph.validate() && graph.topologicalSort() == QStringList({"source", "scale", "shift"}),
                 "MnaOpSchema accepts the wired node and rejects a missing required attribute; graph order source -> scale -> shift");

    //! [mna_param_tree_binding]
    graph.paramTree.setParam("shift/offset", 0.5);
    MnaParamBinding binding;
    binding.targetPath = "scale/gain";
    binding.expression = "clamp(ref('source::out') * 2, 0, 5)"; // evaluated against node results
    binding.trigger = "on_change";
    graph.paramTree.addBinding(binding);
    //! [mna_param_tree_binding]

    //! [mna_graph_execute]
    MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(graph, {});
    const double first = ctx.results.value("shift::out").toDouble(); // "nodeId::port"
    const double gainAfterFirst = graph.paramTree.param("scale/gain").toDouble();

    graph.node("source").attributes["value"] = 1.0;
    graph.node("source").dirty = true; // only source and its downstream nodes run again
    const double second = MnaGraphExecutor::executeIncremental(graph, ctx).results.value("shift::out").toDouble();
    //! [mna_graph_execute]
    // Before execution the binding has no source::out yet, so gain stays 2: 2 * 3 + 0.5 = 6.5.
    // After it, gain = clamp(3 * 2, 0, 5) = 5, so the incremental run gives 5 * 1 + 0.5 = 5.5 and re-binds gain to 2.
    ok &= expect(first == 6.5 && gainAfterFirst == 5.0 && second == 5.5 && graph.paramTree.param("scale/gain").toDouble() == 2.0,
                 QString("MnaGraphExecutor: shift::out = %1, then %2 after the binding sets gain to %3").arg(first).arg(second).arg(gainAfterFirst));

    //! [mna_project_save]
    MnaProject project;
    project.name = "ex_mna";
    project.pipeline = graph.nodes();
    const QString jsonPath = dir.filePath("pipeline.mna"); // UTF-8 JSON
    const QString cborPath = dir.filePath("pipeline.mnx"); // CBOR with an "MNX1" magic
    const bool saved = MnaIO::write(project, jsonPath) && MnaIO::write(project, cborPath);
    const MnaProject fromJson = MnaIO::read(jsonPath);
    const MnaProject fromCbor = MnaIO::read(cborPath);
    //! [mna_project_save]
    ok &= expect(saved && fromJson.name == "ex_mna" && fromJson.pipeline.size() == 3 && fromCbor.pipeline.size() == 3 && fromCbor.pipeline[1].inputs[0].sourceNodeId == "source" && fromJson.pipeline[1].attributes.value("gain").toDouble() == 5.0,
                 "MnaIO round-trips the 3-node pipeline through .mna and .mnx");

    qInfo().noquote() << (ok ? "All mna checks passed." : "mna checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
