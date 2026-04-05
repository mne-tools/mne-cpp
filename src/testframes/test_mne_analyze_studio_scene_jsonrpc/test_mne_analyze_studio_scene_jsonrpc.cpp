//=============================================================================================================
/**
 * @file     test_mne_analyze_studio_scene_jsonrpc.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     June, 2026
 *
 * @brief    Unit tests for JsonRpcMessage, SceneContextRegistry, and ViewManager.
 */

#include <QtTest/QtTest>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <core/fiffbuffer.h>
#include <core/jsonrpcmessage.h>
#include <core/manifestparser.h>
#include <core/mcprouter.h>
#include <core/scenecontextregistry.h>
#include <core/viewmanager.h>
#include <core/viewproviderregistry.h>

using namespace MNEANALYZESTUDIO;

class TestAnalyzeStudioSceneJsonRpc : public QObject
{
    Q_OBJECT

private slots:
    // ── JsonRpcMessage ──────────────────────────────────────────────────────

    void testCreateRequest()
    {
        QJsonObject params{{"channel", "MEG0111"}};
        QJsonObject msg = JsonRpcMessage::createRequest("req-1", "getChannelData", params);

        QCOMPARE(msg.value("jsonrpc").toString(), QString("2.0"));
        QCOMPARE(msg.value("id").toString(), QString("req-1"));
        QCOMPARE(msg.value("method").toString(), QString("getChannelData"));
        QCOMPARE(msg.value("params").toObject().value("channel").toString(), QString("MEG0111"));
    }

    void testCreateRequestNoParams()
    {
        QJsonObject msg = JsonRpcMessage::createRequest("req-2", "ping");

        QCOMPARE(msg.value("jsonrpc").toString(), QString("2.0"));
        QCOMPARE(msg.value("method").toString(), QString("ping"));
        QVERIFY(msg.value("params").toObject().isEmpty());
    }

    void testCreateResponse()
    {
        QJsonObject result{{"status", "ok"}};
        QJsonObject msg = JsonRpcMessage::createResponse(QJsonValue(42), result);

        QCOMPARE(msg.value("jsonrpc").toString(), QString("2.0"));
        QCOMPARE(msg.value("id").toInt(), 42);
        QCOMPARE(msg.value("result").toObject().value("status").toString(), QString("ok"));
    }

    void testCreateError()
    {
        QJsonObject msg = JsonRpcMessage::createError(QJsonValue(QString("e-1")), -32600, "Invalid Request");

        QCOMPARE(msg.value("jsonrpc").toString(), QString("2.0"));
        QCOMPARE(msg.value("id").toString(), QString("e-1"));

        QJsonObject error = msg.value("error").toObject();
        QCOMPARE(error.value("code").toInt(), -32600);
        QCOMPARE(error.value("message").toString(), QString("Invalid Request"));
    }

    void testSerializeDeserializeRoundTrip()
    {
        QJsonObject original = JsonRpcMessage::createRequest("rt-1", "listFiles");
        QByteArray wire = JsonRpcMessage::serialize(original);
        QVERIFY(wire.endsWith('\n'));

        QJsonObject parsed;
        QString errorString;
        bool ok = JsonRpcMessage::deserialize(wire, parsed, errorString);

        QVERIFY2(ok, qPrintable(errorString));
        QCOMPARE(parsed.value("id").toString(), QString("rt-1"));
        QCOMPARE(parsed.value("method").toString(), QString("listFiles"));
    }

    void testDeserializeInvalidJson()
    {
        QJsonObject parsed;
        QString errorString;
        bool ok = JsonRpcMessage::deserialize("{bad json", parsed, errorString);

        QVERIFY(!ok);
    }

    void testDeserializeMissingVersion()
    {
        QByteArray payload = R"({"id": "x", "method": "foo"})";
        QJsonObject parsed;
        QString errorString;
        bool ok = JsonRpcMessage::deserialize(payload, parsed, errorString);

        QVERIFY(!ok);
    }

    void testIsValid()
    {
        QJsonObject valid{{"jsonrpc", "2.0"}, {"id", 1}};
        QJsonObject invalid{{"id", 1}};
        QJsonObject wrong{{"jsonrpc", "1.0"}, {"id", 1}};

        QVERIFY(JsonRpcMessage::isValid(valid));
        QVERIFY(!JsonRpcMessage::isValid(invalid));
        QVERIFY(!JsonRpcMessage::isValid(wrong));
    }

    // ── SceneContextRegistry ────────────────────────────────────────────────

    void testCreateScene()
    {
        SceneContextRegistry reg;
        QString id = reg.createScene("sub01", "Left Hemisphere");

        QVERIFY(!id.isEmpty());
        QVERIFY(id.startsWith("scene_"));
        QCOMPARE(reg.activeSceneForSubject("sub01"), id);
    }

    void testCreateSceneNoSubject()
    {
        SceneContextRegistry reg;
        QString id = reg.createScene("", "Untitled");

        QVERIFY(!id.isEmpty());
        // Empty subjectId skips active-scene insertion, but the fallback
        // loop in activeSceneForSubject still finds the scene by subjectId match.
        QCOMPARE(reg.activeSceneForSubject(""), id);
    }

    void testEnsureSceneReusesExisting()
    {
        SceneContextRegistry reg;
        QString first = reg.createScene("sub01", "Scene A");
        QString reused = reg.ensureScene("sub01", "Scene B");

        QCOMPARE(reused, first);
    }

    void testEnsureSceneCreatesNew()
    {
        SceneContextRegistry reg;
        QString id = reg.ensureScene("sub02", "New Scene");

        QVERIFY(!id.isEmpty());
        QCOMPARE(reg.activeSceneForSubject("sub02"), id);
    }

    void testAddLayerToScene()
    {
        SceneContextRegistry reg;
        QString sceneId = reg.createScene("sub01", "Test");

        bool ok = reg.addLayerToScene(sceneId, "/data/sub01/lh.pial");
        QVERIFY(ok);

        QStringList layers = reg.layersForScene(sceneId);
        QCOMPARE(layers.size(), 1);
        QCOMPARE(layers.first(), QString("/data/sub01/lh.pial"));
    }

    void testAddLayerToInvalidScene()
    {
        SceneContextRegistry reg;
        bool ok = reg.addLayerToScene("nonexistent", "/data/file.surf");
        QVERIFY(!ok);
    }

    void testLayerMoveBetweenScenes()
    {
        SceneContextRegistry reg;
        QString scene1 = reg.createScene("sub01", "Scene 1");
        QString scene2 = reg.createScene("sub01", "Scene 2");

        reg.addLayerToScene(scene1, "/data/overlay.surf");
        QCOMPARE(reg.sceneForLayer("/data/overlay.surf"), scene1);

        reg.addLayerToScene(scene2, "/data/overlay.surf");
        QCOMPARE(reg.sceneForLayer("/data/overlay.surf"), scene2);
        QVERIFY(!reg.layersForScene(scene1).contains("/data/overlay.surf"));
    }

    void testSceneForLayerNotFound()
    {
        SceneContextRegistry reg;
        QVERIFY(reg.sceneForLayer("/nonexistent").isEmpty());
    }

    void testSerializeRestore()
    {
        SceneContextRegistry source;
        QString id1 = source.createScene("sub01", "Original");
        source.addLayerToScene(id1, "/data/lh.pial");
        source.addLayerToScene(id1, "/data/rh.pial");

        QJsonArray serialized = source.serialize();
        QCOMPARE(serialized.size(), 1);

        SceneContextRegistry target;
        target.restore(serialized);

        QCOMPARE(target.activeSceneForSubject("sub01"), id1);
        QStringList layers = target.layersForScene(id1);
        QCOMPARE(layers.size(), 2);
        QVERIFY(layers.contains("/data/lh.pial"));
        QVERIFY(layers.contains("/data/rh.pial"));
    }

    void testRestoreClearsExisting()
    {
        SceneContextRegistry reg;
        reg.createScene("old-subject", "Old Scene");

        reg.restore(QJsonArray());

        QVERIFY(reg.activeSceneForSubject("old-subject").isEmpty());
    }

    // ── ViewManager ─────────────────────────────────────────────────────────

    void testViewKindForFile3D()
    {
        QCOMPARE(ViewManager::viewKindForFile("/data/lh.pial"),    ViewManager::ViewKind::ThreeDScene);
        QCOMPARE(ViewManager::viewKindForFile("/data/rh.white"),   ViewManager::ViewKind::ThreeDScene);
        QCOMPARE(ViewManager::viewKindForFile("/data/inner.surf"), ViewManager::ViewKind::ThreeDScene);
        QCOMPARE(ViewManager::viewKindForFile("/data/head.bem"),   ViewManager::ViewKind::ThreeDScene);
        QCOMPARE(ViewManager::viewKindForFile("/data/lh.inflated"),ViewManager::ViewKind::ThreeDScene);
        QCOMPARE(ViewManager::viewKindForFile("/data/lh.orig"),    ViewManager::ViewKind::ThreeDScene);
    }

    void testViewKindForFileText()
    {
        QCOMPARE(ViewManager::viewKindForFile("/data/script.py"),   ViewManager::ViewKind::TextEditor);
        QCOMPARE(ViewManager::viewKindForFile("/data/main.cpp"),    ViewManager::ViewKind::TextEditor);
        QCOMPARE(ViewManager::viewKindForFile("/data/config.json"), ViewManager::ViewKind::TextEditor);
        QCOMPARE(ViewManager::viewKindForFile("/data/notes.md"),    ViewManager::ViewKind::TextEditor);
        QCOMPARE(ViewManager::viewKindForFile("/data/header.h"),    ViewManager::ViewKind::TextEditor);
        QCOMPARE(ViewManager::viewKindForFile("/data/lib.hpp"),     ViewManager::ViewKind::TextEditor);
        QCOMPARE(ViewManager::viewKindForFile("/data/paper.tex"),   ViewManager::ViewKind::TextEditor);
        QCOMPARE(ViewManager::viewKindForFile("/data/setup.mne"),   ViewManager::ViewKind::TextEditor);
    }

    void testViewKindForFileUnsupported()
    {
        QCOMPARE(ViewManager::viewKindForFile("/data/image.png"),   ViewManager::ViewKind::Unsupported);
        QCOMPARE(ViewManager::viewKindForFile("/data/movie.avi"),   ViewManager::ViewKind::Unsupported);
        QCOMPARE(ViewManager::viewKindForFile("/data/archive.zip"), ViewManager::ViewKind::Unsupported);
    }

    void testDispatchFifUnsupportedByDefault()
    {
        // .fif files have no built-in view mapping; the signal browser
        // is provided through ViewProviderRegistry extensions.
        SceneContextRegistry reg;
        ViewManager vm(&reg);
        QJsonObject dispatch = vm.dispatchFileSelection("/data/sample_audvis_raw.fif");

        QCOMPARE(dispatch.value("view").toString(), QString("Unsupported"));
        QCOMPARE(dispatch.value("mode").toString(), QString("none"));
    }

    void testDispatchThreeDScene()
    {
        SceneContextRegistry reg;
        ViewManager vm(&reg);
        QJsonObject dispatch = vm.dispatchFileSelection("/data/sub01/lh.pial");

        QCOMPARE(dispatch.value("view").toString(), QString("ThreeDView"));
        QCOMPARE(dispatch.value("mode").toString(), QString("merge_or_prompt"));
        QVERIFY(dispatch.contains("agentSuggestion"));
    }

    void testDispatchThreeDSceneWithExistingLayer()
    {
        SceneContextRegistry reg;
        QString sceneId = reg.createScene("lh", "Test");
        reg.addLayerToScene(sceneId, "/data/sub01/lh.pial");

        ViewManager vm(&reg);
        QJsonObject dispatch = vm.dispatchFileSelection("/data/sub01/lh.pial");

        QCOMPARE(dispatch.value("view").toString(), QString("ThreeDView"));
        QCOMPARE(dispatch.value("mode").toString(), QString("restore_scene"));
        QCOMPARE(dispatch.value("sceneId").toString(), sceneId);
    }

    void testDispatchTextEditor()
    {
        SceneContextRegistry reg;
        ViewManager vm(&reg);
        QJsonObject dispatch = vm.dispatchFileSelection("/data/script.py");

        QCOMPARE(dispatch.value("view").toString(), QString("CodeEditorView"));
        QCOMPARE(dispatch.value("mode").toString(), QString("replace_tab"));
    }

    void testDispatchUnsupported()
    {
        SceneContextRegistry reg;
        ViewManager vm(&reg);
        QJsonObject dispatch = vm.dispatchFileSelection("/data/file.xyz");

        QCOMPARE(dispatch.value("view").toString(), QString("Unsupported"));
        QCOMPARE(dispatch.value("mode").toString(), QString("none"));
    }

    void testDispatchWithMetadataSubject()
    {
        SceneContextRegistry reg;
        ViewManager vm(&reg);
        QJsonObject meta{{"subject", "sub-01"}};
        QJsonObject dispatch = vm.dispatchFileSelection("/data/lh.pial", meta);

        QCOMPARE(dispatch.value("subjectId").toString(), QString("sub-01"));
    }

    void testDispatchNoSceneRegistry()
    {
        ViewManager vm(nullptr);
        QJsonObject dispatch = vm.dispatchFileSelection("/data/lh.pial");

        QCOMPARE(dispatch.value("view").toString(), QString("ThreeDView"));
        QCOMPARE(dispatch.value("mode").toString(), QString("merge_or_prompt"));
    }

    // ── McpRouter ───────────────────────────────────────────────────────────

    void testMcpRouterRegisterAndRoute()
    {
        McpRouter router;
        router.registerMethod("ping", [](const QJsonObject&) {
            return QJsonObject{{"pong", true}};
        });

        QJsonObject request = JsonRpcMessage::createRequest("r1", "ping");
        QJsonObject response = router.route(request);

        QCOMPARE(response.value("jsonrpc").toString(), QString("2.0"));
        QCOMPARE(response.value("id").toString(), QString("r1"));
        QCOMPARE(response.value("result").toObject().value("pong").toBool(), true);
    }

    void testMcpRouterUnknownMethod()
    {
        McpRouter router;
        QJsonObject request = JsonRpcMessage::createRequest("r2", "nonexistent");
        QJsonObject response = router.route(request);

        QVERIFY(response.contains("error"));
        QCOMPARE(response.value("error").toObject().value("code").toInt(), -32601);
    }

    void testMcpRouterInvalidEnvelope()
    {
        McpRouter router;
        QJsonObject badRequest{{"id", "x"}, {"method", "foo"}};
        QJsonObject response = router.route(badRequest);

        QVERIFY(response.contains("error"));
        QCOMPARE(response.value("error").toObject().value("code").toInt(), -32600);
    }

    void testMcpRouterPassesParams()
    {
        McpRouter router;
        router.registerMethod("add", [](const QJsonObject& params) {
            int a = params.value("a").toInt();
            int b = params.value("b").toInt();
            return QJsonObject{{"sum", a + b}};
        });

        QJsonObject params{{"a", 3}, {"b", 7}};
        QJsonObject request = JsonRpcMessage::createRequest("r3", "add", params);
        QJsonObject response = router.route(request);

        QCOMPARE(response.value("result").toObject().value("sum").toInt(), 10);
    }

    // ── ManifestParser ──────────────────────────────────────────────────────

    void testManifestParserFileNotFound()
    {
        ManifestParser parser;
        QString errorMsg;
        ExtensionManifest manifest = parser.parseFile("/nonexistent/manifest.json", &errorMsg);

        QVERIFY(manifest.id.isEmpty());
        QVERIFY(!errorMsg.isEmpty());
        QVERIFY(errorMsg.contains("Could not open"));
    }

    void testManifestParserInvalidJson()
    {
        QTemporaryFile tmp;
        QVERIFY(tmp.open());
        tmp.write("{not valid json");
        tmp.flush();

        ManifestParser parser;
        QString errorMsg;
        ExtensionManifest manifest = parser.parseFile(tmp.fileName(), &errorMsg);

        QVERIFY(manifest.id.isEmpty());
        QVERIFY(errorMsg.contains("Invalid manifest JSON"));
    }

    void testManifestParserFullManifest()
    {
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/test_manifest_XXXXXX.json");
        QVERIFY(tmp.open());

        QJsonObject manifest{
            {"id", "com.example.test"},
            {"display_name", "Test Extension"},
            {"version", "1.2.3"},
            {"entry_point", "main.py"},
            {"contributes", QJsonObject{
                {"view_providers", QJsonArray{
                    QJsonObject{
                        {"id", "vp1"},
                        {"display_name", "Test View"},
                        {"widget_type", "custom"},
                        {"slot", "right"},
                        {"supports_scene_merging", true},
                        {"file_extensions", QJsonArray{".stc", ".w"}}
                    }
                }},
                {"tools", QJsonArray{
                    QJsonObject{
                        {"name", "compute_stc"},
                        {"description", "Compute source time courses"},
                        {"input_schema", QJsonObject{{"type", "object"}}},
                        {"result_schema", QJsonObject{{"type", "object"}}}
                    }
                }},
                {"result_renderers", QJsonArray{
                    QJsonObject{
                        {"id", "rr1"},
                        {"display_name", "STC Renderer"},
                        {"tool_names", QJsonArray{"compute_stc"}},
                        {"widget_type", "result_renderer"}
                    }
                }},
                {"analysis_pipelines", QJsonArray{
                    QJsonObject{
                        {"id", "pipe1"},
                        {"display_name", "MEG Pipeline"},
                        {"description", "Full MEG analysis"},
                        {"input_schema", QJsonObject{{"type", "object"}}},
                        {"output_schema", QJsonObject{{"type", "object"}}},
                        {"steps", QJsonArray{QJsonObject{{"tool", "compute_stc"}}}},
                        {"follow_up_actions", QJsonArray{QJsonObject{{"action", "show"}}}}
                    }
                }},
                {"ui", QJsonObject{
                    {"sidebar_items", QJsonArray{"item1", "item2"}},
                    {"menu_items", QJsonArray{"menu1"}},
                    {"settings_tabs", QJsonArray{
                        QJsonObject{
                            {"id", "tab1"},
                            {"title", "Settings"},
                            {"description", "Test settings"},
                            {"fields", QJsonArray{QJsonObject{{"name", "field1"}}}},
                            {"actions", QJsonArray{QJsonObject{{"name", "act1"}}}}
                        }
                    }}
                }}
            }}
        };

        tmp.write(QJsonDocument(manifest).toJson());
        tmp.flush();

        ManifestParser parser;
        QString errorMsg;
        ExtensionManifest result = parser.parseFile(tmp.fileName(), &errorMsg);

        QVERIFY2(errorMsg.isEmpty(), qPrintable(errorMsg));
        QCOMPARE(result.id, QString("com.example.test"));
        QCOMPARE(result.displayName, QString("Test Extension"));
        QCOMPARE(result.version, QString("1.2.3"));
        QCOMPARE(result.entryPoint, QString("main.py"));
        QVERIFY(result.isValid());

        QCOMPARE(result.viewProviders.size(), 1);
        QCOMPARE(result.viewProviders[0].id, QString("vp1"));
        QCOMPARE(result.viewProviders[0].supportsSceneMerging, true);
        QCOMPARE(result.viewProviders[0].fileExtensions.size(), 2);

        QCOMPARE(result.tools.size(), 1);
        QCOMPARE(result.tools[0].name, QString("compute_stc"));

        QCOMPARE(result.resultRenderers.size(), 1);
        QCOMPARE(result.resultRenderers[0].id, QString("rr1"));
        QCOMPARE(result.resultRenderers[0].toolNames.size(), 1);

        QCOMPARE(result.analysisPipelines.size(), 1);
        QCOMPARE(result.analysisPipelines[0].id, QString("pipe1"));

        QCOMPARE(result.ui.sidebarItems.size(), 2);
        QCOMPARE(result.ui.menuItems.size(), 1);
        QCOMPARE(result.ui.settingsTabs.size(), 1);
        QCOMPARE(result.ui.settingsTabs[0].id, QString("tab1"));
    }

    void testManifestParserMissingId()
    {
        QTemporaryFile tmp;
        QVERIFY(tmp.open());
        QJsonObject manifest{{"version", "1.0"}};
        tmp.write(QJsonDocument(manifest).toJson());
        tmp.flush();

        ManifestParser parser;
        QString errorMsg;
        ExtensionManifest result = parser.parseFile(tmp.fileName(), &errorMsg);

        QVERIFY(result.id.isEmpty());
        QVERIFY(errorMsg.contains("missing an id"));
    }

    // ── FiffBuffer ──────────────────────────────────────────────────────────

    void testFiffBufferProperties()
    {
        QString sampleFile = QString("%1/%2").arg(
            qgetenv("MNE_SAMPLE_DATA_PATH").constData(),
            "MEG/sample/sample_audvis_trunc_raw.fif");

        if(!QFile::exists(sampleFile)) {
            sampleFile = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis_trunc_raw.fif";
        }

        if(!QFile::exists(sampleFile)) {
            QSKIP("MNE sample data not found");
        }

        FiffBuffer buf(sampleFile);
        QCOMPARE(buf.kind(), IBuffer::BufferKind::Fiff);
        QCOMPARE(buf.uri(), sampleFile);
        QVERIFY(!buf.isOpen());

        bool opened = buf.open();
        QVERIFY(opened);
        QVERIFY(buf.isOpen());

        // Opening again should return true immediately
        QVERIFY(buf.open());

        QJsonObject meta = buf.getMetadata();
        QVERIFY(!meta.isEmpty());
        QCOMPARE(meta.value("bufferKind").toString(), QString("fiff"));

        QVERIFY(buf.device() != nullptr);

        const FIFFLIB::FiffRawData& raw = buf.rawData();
        QVERIFY(raw.info.nchan > 0);
    }

    // ── ViewProviderRegistry ────────────────────────────────────────────────

    void testViewProviderRegistryEmptyDirectory()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ViewProviderRegistry reg;
        QString errorMsg;
        bool ok = reg.loadFromDirectory(tmpDir.path(), &errorMsg);

        QVERIFY(!ok);
        QVERIFY(reg.allManifests().isEmpty());
    }

    void testViewProviderRegistryInvalidPath()
    {
        ViewProviderRegistry reg;
        QString errorMsg;
        bool ok = reg.loadFromDirectory("/nonexistent/path", &errorMsg);

        QVERIFY(!ok);
        QVERIFY(errorMsg.contains("not found"));
    }

    void testViewProviderRegistryLoadManifest()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Create a subdirectory with a manifest.json
        QDir dir(tmpDir.path());
        QVERIFY(dir.mkpath("test_ext"));

        QFile manifestFile(dir.filePath("test_ext/manifest.json"));
        QVERIFY(manifestFile.open(QIODevice::WriteOnly));

        QJsonObject manifest{
            {"id", "com.test.ext1"},
            {"display_name", "Test Ext 1"},
            {"version", "0.1.0"},
            {"contributes", QJsonObject{
                {"view_providers", QJsonArray{
                    QJsonObject{
                        {"id", "vp_test"},
                        {"display_name", "Test Provider"},
                        {"file_extensions", QJsonArray{".stc", ".w"}}
                    }
                }},
                {"tools", QJsonArray{
                    QJsonObject{
                        {"name", "my_tool"},
                        {"description", "A test tool"}
                    }
                }}
            }}
        };

        manifestFile.write(QJsonDocument(manifest).toJson());
        manifestFile.close();

        ViewProviderRegistry reg;
        QString errorMsg;
        bool ok = reg.loadFromDirectory(tmpDir.path(), &errorMsg);

        QVERIFY2(ok, qPrintable(errorMsg));
        QCOMPARE(reg.allManifests().size(), 1);
        QCOMPARE(reg.allManifests()[0].id, QString("com.test.ext1"));
        QCOMPARE(reg.allManifests()[0].viewProviders.size(), 1);

        QVERIFY(reg.isExtensionEnabled("com.test.ext1"));
        QCOMPARE(reg.manifests().size(), 1);

        QJsonArray tools = reg.toolDefinitions();
        QVERIFY(!tools.isEmpty());
    }

    void testViewProviderRegistryDisableExtension()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QDir dir(tmpDir.path());
        QVERIFY(dir.mkpath("ext_a"));

        QFile manifestFile(dir.filePath("ext_a/manifest.json"));
        QVERIFY(manifestFile.open(QIODevice::WriteOnly));
        QJsonObject manifest{{"id", "ext.a"}, {"version", "1.0"}};
        manifestFile.write(QJsonDocument(manifest).toJson());
        manifestFile.close();

        ViewProviderRegistry reg;
        reg.loadFromDirectory(tmpDir.path());
        QCOMPARE(reg.manifests().size(), 1);

        reg.setDisabledExtensionIds({"ext.a"});
        QCOMPARE(reg.disabledExtensionIds().size(), 1);
        QVERIFY(!reg.isExtensionEnabled("ext.a"));
        QCOMPARE(reg.manifests().size(), 0);
        QCOMPARE(reg.allManifests().size(), 1);
    }

    void testViewProviderRegistryProviderForFile()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QDir dir(tmpDir.path());
        QVERIFY(dir.mkpath("ext_b"));

        QFile manifestFile(dir.filePath("ext_b/manifest.json"));
        QVERIFY(manifestFile.open(QIODevice::WriteOnly));
        QJsonObject manifest{
            {"id", "ext.b"},
            {"contributes", QJsonObject{
                {"view_providers", QJsonArray{
                    QJsonObject{
                        {"id", "custom_viewer"},
                        {"display_name", "Custom Viewer"},
                        {"widget_type", "custom_widget"},
                        {"file_extensions", QJsonArray{".xyz"}}
                    }
                }}
            }}
        };
        manifestFile.write(QJsonDocument(manifest).toJson());
        manifestFile.close();

        ViewProviderRegistry reg;
        reg.loadFromDirectory(tmpDir.path());

        QJsonObject provider = reg.providerForFile("/data/test.xyz");
        QVERIFY(!provider.isEmpty());

        QJsonObject noProvider = reg.providerForFile("/data/test.abc");
        QVERIFY(noProvider.isEmpty());
    }

    void testViewManagerDispatchWithExtension()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QDir dir(tmpDir.path());
        QVERIFY(dir.mkpath("ext_c"));

        QFile manifestFile(dir.filePath("ext_c/manifest.json"));
        QVERIFY(manifestFile.open(QIODevice::WriteOnly));
        QJsonObject manifest{
            {"id", "ext.c"},
            {"contributes", QJsonObject{
                {"view_providers", QJsonArray{
                    QJsonObject{
                        {"id", "stc_viewer"},
                        {"display_name", "STC Viewer"},
                        {"file_extensions", QJsonArray{".stc"}},
                        {"supports_scene_merging", true}
                    }
                }}
            }}
        };
        manifestFile.write(QJsonDocument(manifest).toJson());
        manifestFile.close();

        SceneContextRegistry sceneReg;
        ViewProviderRegistry vpReg;
        vpReg.loadFromDirectory(tmpDir.path());

        ViewManager vm(&sceneReg, &vpReg);
        QJsonObject dispatch = vm.dispatchFileSelection("/data/sub01/lh.stc");

        QCOMPARE(dispatch.value("view").toString(), QString("ExtensionView"));
        QCOMPARE(dispatch.value("mode").toString(), QString("merge_or_prompt"));
    }
};

QTEST_GUILESS_MAIN(TestAnalyzeStudioSceneJsonRpc)
#include "test_mne_analyze_studio_scene_jsonrpc.moc"
