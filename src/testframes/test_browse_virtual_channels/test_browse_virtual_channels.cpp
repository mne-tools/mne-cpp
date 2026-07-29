//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_browse_virtual_channels.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks the mne_browse virtual channel model.
 *
 * A virtual channel is a derivation: a source channel minus one or more
 * references, optionally weighted. The model decides what that derivation
 * actually is, and a wrong decision does not announce itself. A reference
 * silently dropped, a weight paired with the wrong channel, or a channel
 * referencing itself all produce a trace that still draws and is simply not
 * the signal the user asked for.
 *
 * That makes the normalisation rules worth pinning rather than the widget
 * mechanics. The rules are: names and channels are trimmed, a channel cannot
 * reference itself, duplicate references collapse, and a weighted derivation
 * gets one weight per surviving reference whether or not the caller supplied
 * enough of them.
 *
 * The definitions also round trip through a file, so a saved montage that
 * loads back as something different is checked for directly.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Models/virtualchannelmodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;

//=============================================================================================================
/**
 * DECLARE CLASS TestBrowseVirtualChannels
 *
 * @brief Checks the derivation rules of the mne_browse virtual channel model.
 */
class TestBrowseVirtualChannels: public QObject
{
    Q_OBJECT

public:
    TestBrowseVirtualChannels() = default;

private slots:
    void freshModel_isEmpty();

    void addChannel_returnsUsableRow();
    void references_areTrimmed();
    void references_cannotIncludeThePrimaryChannel();
    void references_areDeduplicated();

    void weights_matchReferenceCount_data();
    void weights_matchReferenceCount();

    void removeRows_rejectsOutOfRange_data();
    void removeRows_rejectsOutOfRange();
    void removeRows_removesTheRightOne();

    void referenceSet_roundTripsAndResolves();
    void referenceSet_removingAnUnknownOneIsRejected();

    void data_outOfRangeIndexIsInvalid();

    void saveAndLoad_roundTripsDefinitions();
    void load_ofRubbishIsRejected();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestBrowseVirtualChannels::freshModel_isEmpty()
{
    VirtualChannelModel model;

    QCOMPARE(model.rowCount(), 0);
    QVERIFY2(model.columnCount() > 0, "a table model with no columns cannot be displayed");
}

//=============================================================================================================

void TestBrowseVirtualChannels::addChannel_returnsUsableRow()
{
    VirtualChannelModel model;

    // The returned row is used by the caller to select the new entry, so it has
    // to be a real index into the model rather than a count or a status code.
    const int row = model.addVirtualChannel("derived-1", VirtualChannelKind::Bipolar,
                                            "MEG0111", QStringList{"MEG0112"},
                                            QVector<double>(), QString());

    QCOMPARE(row, 0);
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.index(row, 0).isValid());

    const int second = model.addVirtualChannel("derived-2", VirtualChannelKind::Bipolar,
                                               "MEG0113", QStringList{"MEG0114"},
                                               QVector<double>(), QString());
    QCOMPARE(second, 1);
    QCOMPARE(model.rowCount(), 2);
}

//=============================================================================================================

void TestBrowseVirtualChannels::references_areTrimmed()
{
    VirtualChannelModel model;

    // Channel names arrive from a text field or a loaded file, so surrounding
    // whitespace is ordinary. An untrimmed name never matches a real channel
    // and the derivation quietly loses that reference.
    model.addVirtualChannel("  derived  ", VirtualChannelKind::Bipolar,
                            "  MEG0111  ", QStringList{"  MEG0112  ", "\tMEG0113\t"},
                            QVector<double>(), QString());

    const QVector<VirtualChannelDefinition> definitions = model.virtualChannels();
    QCOMPARE(definitions.size(), 1);

    QCOMPARE(definitions.at(0).primaryChannel, QString("MEG0111"));
    QCOMPARE(definitions.at(0).referenceChannels.size(), 2);
    QCOMPARE(definitions.at(0).referenceChannels.at(0), QString("MEG0112"));
    QCOMPARE(definitions.at(0).referenceChannels.at(1), QString("MEG0113"));
}

//=============================================================================================================

void TestBrowseVirtualChannels::references_cannotIncludeThePrimaryChannel()
{
    VirtualChannelModel model;

    // A channel referencing itself subtracts its own signal and yields a flat
    // trace. That looks like a dead sensor rather than like a bad montage, so
    // it has to be refused at definition time.
    model.addVirtualChannel("self", VirtualChannelKind::Bipolar,
                            "MEG0111", QStringList{"MEG0111", "MEG0112"},
                            QVector<double>(), QString());

    const QVector<VirtualChannelDefinition> definitions = model.virtualChannels();
    QCOMPARE(definitions.size(), 1);

    QVERIFY2(!definitions.at(0).referenceChannels.contains("MEG0111"),
             "a channel was allowed to reference itself, which flattens its own trace");
    QCOMPARE(definitions.at(0).referenceChannels, QStringList{"MEG0112"});
}

//=============================================================================================================

void TestBrowseVirtualChannels::references_areDeduplicated()
{
    VirtualChannelModel model;

    // A duplicated reference would be subtracted twice, scaling the result by
    // an amount nobody asked for.
    model.addVirtualChannel("dupes", VirtualChannelKind::Bipolar,
                            "MEG0111", QStringList{"MEG0112", "MEG0112", "MEG0113", "MEG0112"},
                            QVector<double>(), QString());

    const QVector<VirtualChannelDefinition> definitions = model.virtualChannels();
    QCOMPARE(definitions.at(0).referenceChannels.size(), 2);
    QCOMPARE(definitions.at(0).referenceChannels.at(0), QString("MEG0112"));
    QCOMPARE(definitions.at(0).referenceChannels.at(1), QString("MEG0113"));
}

//=============================================================================================================

void TestBrowseVirtualChannels::weights_matchReferenceCount_data()
{
    QTest::addColumn<QStringList>("references");
    QTest::addColumn<QVector<double>>("weights");
    QTest::addColumn<int>("expectedReferenceCount");

    // The weights are parallel to the references. A caller can supply too few,
    // too many, or none, and after normalisation there must be exactly one
    // weight per surviving reference. Anything else pairs a weight with the
    // wrong channel, which is a wrong derivation that still plots.
    QTest::newRow("exact")      << QStringList{"A", "B"}      << QVector<double>{0.5, 0.5} << 2;
    QTest::newRow("too few")    << QStringList{"A", "B", "C"} << QVector<double>{0.5}      << 3;
    QTest::newRow("too many")   << QStringList{"A"}           << QVector<double>{0.5, 0.25, 0.25} << 1;
    QTest::newRow("none")       << QStringList{"A", "B"}      << QVector<double>()         << 2;
    QTest::newRow("with dupes") << QStringList{"A", "A", "B"} << QVector<double>{1.0, 2.0, 3.0} << 2;
}

//=============================================================================================================

void TestBrowseVirtualChannels::weights_matchReferenceCount()
{
    QFETCH(QStringList, references);
    QFETCH(QVector<double>, weights);
    QFETCH(int, expectedReferenceCount);

    VirtualChannelModel model;
    model.addVirtualChannel("weighted", VirtualChannelKind::WeightedReference,
                            "PRIMARY", references, weights, QString());

    const QVector<VirtualChannelDefinition> definitions = model.virtualChannels();
    QCOMPARE(definitions.size(), 1);

    QCOMPARE(definitions.at(0).referenceChannels.size(), expectedReferenceCount);
    QVERIFY2(definitions.at(0).referenceWeights.size() == definitions.at(0).referenceChannels.size(),
             qPrintable(QString("%1 references but %2 weights, so at least one weight applies to "
                                "the wrong channel")
                        .arg(definitions.at(0).referenceChannels.size())
                        .arg(definitions.at(0).referenceWeights.size())));
}

//=============================================================================================================

void TestBrowseVirtualChannels::removeRows_rejectsOutOfRange_data()
{
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("count");

    // The model holds two rows in the test below. Every one of these would
    // index outside it, and a caller acts on the return value.
    QTest::newRow("negative position") << -1 <<  1;
    QTest::newRow("zero rows")         <<  0 <<  0;
    QTest::newRow("negative rows")     <<  0 << -1;
    QTest::newRow("past the end")      <<  2 <<  1;
    QTest::newRow("straddles the end") <<  1 <<  5;
    QTest::newRow("far past the end")  << 99 <<  1;
}

//=============================================================================================================

void TestBrowseVirtualChannels::removeRows_rejectsOutOfRange()
{
    QFETCH(int, position);
    QFETCH(int, count);

    VirtualChannelModel model;
    model.addVirtualChannel("a", VirtualChannelKind::Bipolar, "P1", QStringList{"R1"},
                            QVector<double>(), QString());
    model.addVirtualChannel("b", VirtualChannelKind::Bipolar, "P2", QStringList{"R2"},
                            QVector<double>(), QString());

    QVERIFY2(!model.removeRows(position, count),
             "an out of range removal reported success, so the caller believes rows are gone");

    // Nothing was removed on the way to refusing.
    QCOMPARE(model.rowCount(), 2);
}

//=============================================================================================================

void TestBrowseVirtualChannels::removeRows_removesTheRightOne()
{
    VirtualChannelModel model;
    model.addVirtualChannel("first", VirtualChannelKind::Bipolar, "P1", QStringList{"R1"},
                            QVector<double>(), QString());
    model.addVirtualChannel("second", VirtualChannelKind::Bipolar, "P2", QStringList{"R2"},
                            QVector<double>(), QString());
    model.addVirtualChannel("third", VirtualChannelKind::Bipolar, "P3", QStringList{"R3"},
                            QVector<double>(), QString());

    QVERIFY(model.removeRows(1, 1));
    QCOMPARE(model.rowCount(), 2);

    // Removing the middle entry has to leave the outer two, in order. An off by
    // one here deletes a montage entry the user did not select.
    const QVector<VirtualChannelDefinition> definitions = model.virtualChannels();
    QCOMPARE(definitions.at(0).primaryChannel, QString("P1"));
    QCOMPARE(definitions.at(1).primaryChannel, QString("P3"));
}

//=============================================================================================================

void TestBrowseVirtualChannels::referenceSet_roundTripsAndResolves()
{
    VirtualChannelModel model;

    model.addReferenceSet("mastoids", QStringList{"A1", "A2"});

    // A definition naming a set but listing no references resolves through the
    // set. If that resolution failed the derivation would have no reference at
    // all and would show the raw channel instead.
    model.addVirtualChannel("via-set", VirtualChannelKind::AverageReference,
                            "MEG0111", QStringList(), QVector<double>(), "mastoids");

    const QVector<VirtualChannelDefinition> definitions = model.virtualChannels();
    QCOMPARE(definitions.size(), 1);
    QCOMPARE(definitions.at(0).referenceChannels.size(), 2);
    QCOMPARE(definitions.at(0).referenceChannels.at(0), QString("A1"));
    QCOMPARE(definitions.at(0).referenceChannels.at(1), QString("A2"));
}

//=============================================================================================================

void TestBrowseVirtualChannels::referenceSet_removingAnUnknownOneIsRejected()
{
    VirtualChannelModel model;
    model.addReferenceSet("mastoids", QStringList{"A1", "A2"});

    QVERIFY2(!model.removeReferenceSet("no-such-set"),
             "removing a set that was never added reported success");

    // The real one is untouched by the failed removal.
    QVERIFY(model.removeReferenceSet("mastoids"));
}

//=============================================================================================================

void TestBrowseVirtualChannels::data_outOfRangeIndexIsInvalid()
{
    VirtualChannelModel model;
    model.addVirtualChannel("only", VirtualChannelKind::Bipolar, "P1", QStringList{"R1"},
                            QVector<double>(), QString());

    // A view can ask for any index while a model is changing underneath it, so
    // out of range has to come back invalid rather than reading past the end.
    QVERIFY(!model.data(model.index(5, 0)).isValid());
    QVERIFY(!model.data(model.index(0, 99)).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());
}

//=============================================================================================================

void TestBrowseVirtualChannels::saveAndLoad_roundTripsDefinitions()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("montage.json");

    {
        VirtualChannelModel model;
        model.addReferenceSet("mastoids", QStringList{"A1", "A2"});
        model.addVirtualChannel("bipolar", VirtualChannelKind::Bipolar,
                                "MEG0111", QStringList{"MEG0112"},
                                QVector<double>(), QString());
        model.addVirtualChannel("weighted", VirtualChannelKind::WeightedReference,
                                "MEG0113", QStringList{"MEG0114", "MEG0115"},
                                QVector<double>{0.25, 0.75}, QString());

        QFile file(path);
        QVERIFY2(model.saveVirtualChannels(file), "saving the montage failed");
    }

    VirtualChannelModel reloaded;
    QFile file(path);
    QVERIFY2(reloaded.loadVirtualChannels(file), "loading the montage back failed");

    // A montage that loads back as something different is the failure worth
    // catching: the user reopens a session and silently gets other derivations.
    const QVector<VirtualChannelDefinition> definitions = reloaded.virtualChannels();
    QCOMPARE(definitions.size(), 2);

    QCOMPARE(definitions.at(0).primaryChannel, QString("MEG0111"));
    QCOMPARE(definitions.at(0).kind, VirtualChannelKind::Bipolar);
    QCOMPARE(definitions.at(0).referenceChannels, QStringList{"MEG0112"});

    QCOMPARE(definitions.at(1).primaryChannel, QString("MEG0113"));
    QCOMPARE(definitions.at(1).kind, VirtualChannelKind::WeightedReference);
    QCOMPARE(definitions.at(1).referenceChannels.size(), 2);

    // The weights are the part most likely to be lost in serialisation, and
    // losing them turns a weighted derivation into a plain average.
    QCOMPARE(definitions.at(1).referenceWeights.size(), 2);
    QCOMPARE(definitions.at(1).referenceWeights.at(0), 0.25);
    QCOMPARE(definitions.at(1).referenceWeights.at(1), 0.75);
}

//=============================================================================================================

void TestBrowseVirtualChannels::load_ofRubbishIsRejected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // A file that is not a montage is what a user picks by mistake. Loading it
    // has to fail rather than leaving a half populated model.
    const QString path = dir.filePath("not-a-montage.json");
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("this is not json at all");
    }

    VirtualChannelModel model;
    QFile file(path);
    QVERIFY2(!model.loadVirtualChannels(file), "loading a non montage file reported success");
    QCOMPARE(model.rowCount(), 0);

    // A file that does not exist at all.
    QFile missing(dir.filePath("no-such-file.json"));
    QVERIFY2(!model.loadVirtualChannels(missing), "loading a missing file reported success");
    QCOMPARE(model.rowCount(), 0);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestBrowseVirtualChannels)
#include "test_browse_virtual_channels.moc"
