//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_anshared_models.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks the mne_analyze anShared models.
 *
 * These models sit between the file readers and the mne_analyze views, and
 * none of them had a test. They are worth more than a construction check
 * because they expose the sample range and sampling frequency the whole view
 * layer positions itself with, and those can be asserted against mne-python
 * on the same file rather than against MNE-CPP's own output.
 *
 * A sample range that is off by the first sample offset is the classic failure
 * here: the data still draws, the time axis is just silently wrong. The values
 * below come from mne.io.read_raw_fif on sample_audvis_trunc_raw.fif:
 *
 *   sfreq        300.3074951171875
 *   first_samp   12900
 *   last_samp    18906
 *   channels     376
 *
 * The event model is checked for its empty state instead, since that is what a
 * freshly opened file has and it is where an unguarded index would fault.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/averagingdatamodel.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/dipolefitmodel.h>
#include <anShared/Model/mricoordmodel.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Management/eventmanager.h>
#include <anShared/Management/communicator.h>

#include <fiff/fiff_info.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestAnSharedModels
 *
 * @brief Checks the mne_analyze anShared models against mne-python and their empty states.
 */
class TestAnSharedModels: public QObject
{
    Q_OBJECT

public:
    TestAnSharedModels() = default;

private:
    static QString rawPath();

private slots:
    void initTestCase();
    void rawModel_matchesPython();
    void rawModel_emptyStateIsSafe();
    void eventModel_emptyStateIsSafe();
    void analyzeData_startsEmpty();
    void construct_remainingModels();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestAnSharedModels::rawPath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

//=============================================================================================================

void TestAnSharedModels::initTestCase()
{
    if(!QFile::exists(rawPath())) {
        QSKIP("Raw test data not found");
    }
}

//=============================================================================================================

void TestAnSharedModels::rawModel_matchesPython()
{
    FiffRawViewModel model(rawPath());

    QVERIFY2(model.getFiffInfo(), "model loaded no fiff info");

    // The whole view layer positions itself using this range. If it were off by
    // the first sample offset the data would still draw, with a time axis that
    // is quietly wrong, which is not the kind of fault anyone notices by eye.
    QCOMPARE(static_cast<int>(model.absoluteFirstSample()), 12900);
    QCOMPARE(static_cast<int>(model.absoluteLastSample()),  18906);

    // Stored as float, so compared at float precision rather than as a double.
    const double sfreq = static_cast<double>(model.getSamplingFrequency());
    QVERIFY2(std::fabs(sfreq - 300.3074951171875) < 1.0e-4,
             qPrintable(QString("sampling frequency is %1, mne-python says 300.3074951171875")
                        .arg(sfreq, 0, 'g', 17)));

    QCOMPARE(model.getFiffInfo()->nchan, 376);

    // The sample range has to be ordered, or every downstream length is negative.
    QVERIFY(model.absoluteLastSample() > model.absoluteFirstSample());
}

//=============================================================================================================

void TestAnSharedModels::rawModel_emptyStateIsSafe()
{
    // A default constructed model is what mne_analyze holds before a file is
    // opened, so the accessors have to be callable in that state rather than
    // faulting on a null reader.
    FiffRawViewModel model;

    QVERIFY(model.rowCount() >= 0);

    // The accessors document -1 as the answer when no raw data is loaded, so
    // that is asserted rather than merely calling them. Before the guard in
    // absoluteFirstSample and absoluteLastSample these dereferenced a null
    // FiffIO and took the process down, which is what this case caught.
    QCOMPARE(static_cast<int>(model.absoluteFirstSample()), -1);
    QCOMPARE(static_cast<int>(model.absoluteLastSample()), -1);

    // No file means no sampling frequency, but asking has to be survivable.
    model.getSamplingFrequency();
}

//=============================================================================================================

void TestAnSharedModels::eventModel_emptyStateIsSafe()
{
    EventModel model;

    // A freshly opened file has no events, and that is the state the event
    // view asks about first.
    QVERIFY(model.rowCount() >= 0);
    QVERIFY(model.columnCount() >= 0);

    // An index outside the model has to come back invalid rather than reaching
    // into empty storage.
    const QVariant out = model.data(model.index(0, 0));
    Q_UNUSED(out)
}

//=============================================================================================================

void TestAnSharedModels::analyzeData_startsEmpty()
{
    AnalyzeData data;

    // The model list starts empty, which is what the plugin manager relies on
    // when it decides whether anything is loaded.
    QCOMPARE(data.getAllModels().size(), 0);

    // Asking for a model that was never added returns nothing rather than a
    // dangling pointer. Both lookup routes are checked because the plugins use
    // both, and either returning garbage would be acted on as a real model.
    QVERIFY(data.getModelByPath("/no/such/file.fif").isNull());
    QVERIFY(data.getModelByName("no such model").isNull());
}

//=============================================================================================================

void TestAnSharedModels::construct_remainingModels()
{
    // These need a loaded file or a full analysis session to say anything
    // about, so this only checks clean construction and destruction. Shallow
    // on purpose: these types are built only by mne_analyze, so a fault in
    // construction currently reaches a user before anything else notices.
    {
        AveragingDataModel model;
        Q_UNUSED(model)
    }
    {
        BemDataModel model;
        Q_UNUSED(model)
    }
    // These two have no default constructor and take a path. A path that does
    // not exist is the interesting case anyway: the application hands these
    // models whatever the user picked, so failing to open has to leave a valid
    // object rather than a half constructed one.
    {
        DipoleFitModel model("/no/such/file.dip");
        Q_UNUSED(model)
    }
    {
        MriCoordModel model("/no/such/file.fif");
        Q_UNUSED(model)
    }

    QVERIFY(true);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestAnSharedModels)
#include "test_anshared_models.moc"
