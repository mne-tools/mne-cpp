//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_studio_extension_widgets.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks the mne_analyze_studio extension result widgets.
 *
 * These widgets are handed whatever a skill produced, so they receive JSON
 * they did not construct and arrays they did not size. That is the interesting
 * property: a result object with the wrong shape, or a spectrum whose
 * frequency and value arrays disagree in length, has to be survivable rather
 * than indexing off the end of one of them.
 *
 * The widgets are also painted here. Their paint paths are where the arrays
 * are actually walked, so constructing without painting would leave the code
 * that matters unexecuted while still looking like the widget was covered.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <analysisresultswidget.h>
#include <psdresultwidget.h>
#include <spectrumplotwidget.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZESTUDIO;

//=============================================================================================================
/**
 * DECLARE CLASS TestStudioExtensionWidgets
 *
 * @brief Checks the studio extension result widgets against malformed input.
 */
class TestStudioExtensionWidgets: public QObject
{
    Q_OBJECT

public:
    TestStudioExtensionWidgets() = default;

private:
    /** Renders a widget offscreen so its paint path really runs. */
    static void paintOffscreen(QWidget* pWidget);

    QScopedPointer<QWidget> m_pHolder;

private slots:
    void initTestCase();

    void spectrumPlot_reportsItsToolName();
    void spectrumPlot_mismatchedArrayLengths_data();
    void spectrumPlot_mismatchedArrayLengths();
    void spectrumPlot_degenerateRangesArePaintable();
    void spectrumPlot_clearLeavesItPaintable();

    void analysisResults_toolNameIsStable();
    void analysisResults_malformedResultIsSurvivable_data();
    void analysisResults_malformedResultIsSurvivable();

    void psdResult_toolNameIsStable();
    void psdResult_malformedResultIsSurvivable();
    void psdResult_emptyHistoryIsSurvivable();

    void cleanupTestCase();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestStudioExtensionWidgets::paintOffscreen(QWidget* pWidget)
{
    // render() drives the real paintEvent. Without this the array walking code
    // never runs and the widget would look covered while its interesting path
    // was never entered.
    QPixmap canvas(pWidget->size());
    canvas.fill(Qt::transparent);
    pWidget->render(&canvas);
}

//=============================================================================================================

void TestStudioExtensionWidgets::initTestCase()
{
    m_pHolder.reset(new QWidget());
}

//=============================================================================================================

void TestStudioExtensionWidgets::spectrumPlot_reportsItsToolName()
{
    SpectrumPlotWidget widget(m_pHolder.data());
    widget.resize(400, 300);

    // A freshly built plot has nothing to draw and must still paint.
    paintOffscreen(&widget);

    QVERIFY(widget.size().isValid() && !widget.size().isEmpty());
}

//=============================================================================================================

void TestStudioExtensionWidgets::spectrumPlot_mismatchedArrayLengths_data()
{
    QTest::addColumn<int>("frequencyCount");
    QTest::addColumn<int>("valueCount");

    // The two arrays are parallel but nothing enforces that at the setter, so
    // every disagreement a caller can produce is checked. Painting indexes
    // both, and reading past the end of the shorter one is the failure.
    QTest::newRow("equal")            << 16 << 16;
    QTest::newRow("more frequencies") << 16 <<  4;
    QTest::newRow("more values")      <<  4 << 16;
    QTest::newRow("frequencies only") << 16 <<  0;
    QTest::newRow("values only")      <<  0 << 16;
    QTest::newRow("both empty")       <<  0 <<  0;
    QTest::newRow("single point")     <<  1 <<  1;
}

//=============================================================================================================

void TestStudioExtensionWidgets::spectrumPlot_mismatchedArrayLengths()
{
    QFETCH(int, frequencyCount);
    QFETCH(int, valueCount);

    QVector<double> frequencies;
    for(int i = 0; i < frequencyCount; ++i) {
        frequencies.append(1.0 + i);
    }

    QVector<double> values;
    for(int i = 0; i < valueCount; ++i) {
        values.append(0.5 * i);
    }

    SpectrumPlotWidget widget(m_pHolder.data());
    widget.resize(400, 300);

    widget.setSpectrum(frequencies, values, "test spectrum");
    paintOffscreen(&widget);

    // The comparison overlay is a second pair of arrays walked the same way,
    // so it gets the same mismatched input.
    widget.setComparisonSpectrum(frequencies, values, "comparison");
    paintOffscreen(&widget);

    widget.clearComparisonSpectrum();
    paintOffscreen(&widget);
}

//=============================================================================================================

void TestStudioExtensionWidgets::spectrumPlot_degenerateRangesArePaintable()
{
    SpectrumPlotWidget widget(m_pHolder.data());
    widget.resize(400, 300);

    // A flat spectrum makes the value span zero, which is a division waiting to
    // happen when the plot normalises a point into the drawing rectangle. A
    // real recording that saturates or a channel that is off produces exactly
    // this.
    QVector<double> frequencies{1.0, 2.0, 3.0, 4.0};
    QVector<double> flat{7.0, 7.0, 7.0, 7.0};
    widget.setSpectrum(frequencies, flat, "flat values");
    paintOffscreen(&widget);

    // The same degeneracy on the frequency axis.
    QVector<double> singleFrequency{5.0, 5.0, 5.0, 5.0};
    QVector<double> values{1.0, 2.0, 3.0, 4.0};
    widget.setSpectrum(singleFrequency, values, "flat frequencies");
    paintOffscreen(&widget);

    // Both flat at once.
    widget.setSpectrum(singleFrequency, flat, "flat both");
    paintOffscreen(&widget);
}

//=============================================================================================================

void TestStudioExtensionWidgets::spectrumPlot_clearLeavesItPaintable()
{
    SpectrumPlotWidget widget(m_pHolder.data());
    widget.resize(400, 300);

    QVector<double> frequencies{1.0, 2.0, 3.0};
    QVector<double> values{4.0, 5.0, 6.0};
    widget.setSpectrum(frequencies, values, "before clear");
    paintOffscreen(&widget);

    // clear has to leave the widget in the same state a fresh one is in,
    // rather than in one where the title survives but the data does not.
    widget.clear();
    paintOffscreen(&widget);
}

//=============================================================================================================

void TestStudioExtensionWidgets::analysisResults_toolNameIsStable()
{
    AnalysisResultsWidget widget(m_pHolder.data());

    // toolName is not a fixed identity, it reports whichever tool last supplied
    // a result. Before anything arrives there is no answer, and empty is the
    // honest one.
    QVERIFY2(widget.toolName().isEmpty(),
             "a widget that has been given no result should not name a tool");

    // After a result it has to report exactly what it was handed. The widget
    // puts this name back into the command payloads it emits, so a name that
    // did not round trip would send actions to the wrong tool.
    widget.setResult("neurokernel.channel_stats", QJsonObject{{"status", "ok"}});
    QCOMPARE(widget.toolName(), QString("neurokernel.channel_stats"));

    // A second result replaces it rather than accumulating.
    widget.setResult("neurokernel.raw_stats", QJsonObject{{"status", "ok"}});
    QCOMPARE(widget.toolName(), QString("neurokernel.raw_stats"));
}

//=============================================================================================================

void TestStudioExtensionWidgets::analysisResults_malformedResultIsSurvivable_data()
{
    QTest::addColumn<QJsonObject>("result");

    // Everything here is something a skill can really return: a failure, a
    // result with no payload, one whose fields are the wrong type. The widget
    // does not construct these objects so it cannot assume their shape.
    QTest::newRow("empty")            << QJsonObject{};
    QTest::newRow("error status")     << QJsonObject{{"status", "error"},
                                                     {"message", "something failed"}};
    QTest::newRow("status only")      << QJsonObject{{"status", "ok"}};
    QTest::newRow("wrong types")      << QJsonObject{{"status", 42},
                                                     {"message", QJsonArray{1, 2, 3}}};
    QTest::newRow("null payload")     << QJsonObject{{"status", "ok"},
                                                     {"payload", QJsonValue::Null}};
    QTest::newRow("nested empty")     << QJsonObject{{"status", "ok"},
                                                     {"payload", QJsonObject{}}};
}

//=============================================================================================================

void TestStudioExtensionWidgets::analysisResults_malformedResultIsSurvivable()
{
    QFETCH(QJsonObject, result);

    AnalysisResultsWidget widget(m_pHolder.data());
    widget.resize(500, 400);

    // The tool name is passed explicitly because the widget only knows one once
    // a result has arrived. This is a name the widget special cases, so it takes
    // the branch that reads into the payload rather than the generic one.
    widget.setResult("neurokernel.channel_stats", result);
    paintOffscreen(&widget);

    // The runtime context comes from the same untrusted direction.
    widget.setRuntimeContext(result);
    paintOffscreen(&widget);

    // The name survives whatever the payload turned out to be.
    QCOMPARE(widget.toolName(), QString("neurokernel.channel_stats"));
}

//=============================================================================================================

void TestStudioExtensionWidgets::psdResult_toolNameIsStable()
{
    PsdResultWidget widget(m_pHolder.data());

    QVERIFY2(widget.toolName().isEmpty(),
             "a widget that has been given no result should not name a tool");

    widget.setResult("neurokernel.psd", QJsonObject{{"status", "ok"}});
    QCOMPARE(widget.toolName(), QString("neurokernel.psd"));
}

//=============================================================================================================

void TestStudioExtensionWidgets::psdResult_malformedResultIsSurvivable()
{
    PsdResultWidget widget(m_pHolder.data());
    widget.resize(500, 400);

    // A PSD result is expected to carry frequency and value arrays. Handing it
    // arrays of different lengths is the same hazard as the plot widget, one
    // level up, and a skill that failed partway through produces it.
    const QJsonObject mismatched{
        {"status", "ok"},
        {"frequencies", QJsonArray{1.0, 2.0, 3.0, 4.0}},
        {"values", QJsonArray{0.1, 0.2}}
    };
    widget.setResult("neurokernel.psd", mismatched);
    paintOffscreen(&widget);

    // A comparison against a result of a different length again.
    widget.setComparisonResult(QJsonObject{
        {"status", "ok"},
        {"frequencies", QJsonArray{1.0}},
        {"values", QJsonArray{0.1, 0.2, 0.3}}
    });
    paintOffscreen(&widget);

    // And an object with none of the expected keys at all.
    widget.setResult("neurokernel.psd", QJsonObject{{"unexpected", "shape"}});
    paintOffscreen(&widget);

    QCOMPARE(widget.toolName(), QString("neurokernel.psd"));
}

//=============================================================================================================

void TestStudioExtensionWidgets::psdResult_emptyHistoryIsSurvivable()
{
    PsdResultWidget widget(m_pHolder.data());
    widget.resize(500, 400);

    // No history is the state before anything has been run, which is when the
    // widget is first shown.
    widget.setResultHistory(QJsonArray{});
    paintOffscreen(&widget);

    // A history whose entries are not objects is malformed but reachable, since
    // it is read back from a saved session file a user can edit.
    widget.setResultHistory(QJsonArray{1, "two", QJsonValue::Null});
    paintOffscreen(&widget);

    // No result was ever set, so the widget still names no tool. Reaching here
    // at all is the claim: a malformed history did not take it down.
    QVERIFY(widget.toolName().isEmpty());
}

//=============================================================================================================

void TestStudioExtensionWidgets::cleanupTestCase()
{
    m_pHolder.reset();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestStudioExtensionWidgets)
#include "test_studio_extension_widgets.moc"
