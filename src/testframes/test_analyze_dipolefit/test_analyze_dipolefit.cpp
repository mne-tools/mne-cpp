//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_dipolefit.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Drives the mne_analyze dipole fit plugin through its control panel.
 *
 * The fit the panel produces with its default settings is compared with the
 * InvDipoleFit library (checked against mne.fit_dipole in
 * test_inv_dipole_fit_python) run with MNE-C's defaults, so a panel that
 * sends wrong units or wrong defaults shows up as a different fit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/dipolefit/dipolefit.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/event.h>
#include <anShared/Model/averagingdatamodel.h>
#include <anShared/Model/dipolefitmodel.h>

#include <inv/dipole_fit/inv_dipole_fit.h>
#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_ecd_set.h>

#include <QtTest>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QPushButton>
#include <QSpinBox>

#include <memory>

using namespace DIPOLEFITPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeDipoleFit : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void defaultPanelFitsLikeMneC();
};

//=============================================================================================================

void TestAnalyzeDipoleFit::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("mne-cpp-tests");
    QCoreApplication::setApplicationName("test_analyze_dipolefit");
}

//=============================================================================================================

void TestAnalyzeDipoleFit::defaultPanelFitsLikeMneC()
{
    const QString avePath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
    if (!QFile::exists(avePath))
        QSKIP("Sample evoked data not found");

    auto data = QSharedPointer<AnalyzeData>::create();
    auto plugin = std::make_unique<InvDipoleFit>();
    plugin->setGlobalData(data);
    plugin->init();
    std::unique_ptr<QDockWidget> control(plugin->getControl());

    auto average = QSharedPointer<AveragingDataModel>::create(avePath);
    plugin->handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(average.staticCast<AbstractModel>())));

    // A user picks the measurement, MEG, 90..100 ms, and presses Fit; everything else stays at its default
    auto* meas = control->findChild<QComboBox*>("comboBox_meas");
    QVERIFY(meas);
    meas->setCurrentText(QFileInfo(avePath).fileName());
    control->findChild<QCheckBox*>("checkBox_MEG")->setChecked(true);
    control->findChild<QSpinBox*>("spinBox_tmax")->setValue(100);
    control->findChild<QSpinBox*>("spinBox_tmin")->setValue(90);
    control->findChild<QPushButton*>("pushButton_fit")->click();

    QSharedPointer<DipoleFitModel> fitModel;
    QTRY_VERIFY_WITH_TIMEOUT(
        [&] {
            for (const auto& model : data->getAllModels()) {
                if (model->getType() == ANSHAREDLIB_DIPOLEFIT_MODEL)
                    fitModel = qSharedPointerCast<DipoleFitModel>(model);
            }
            return !fitModel.isNull();
        }(),
        60000);
    const auto fit = fitModel->data(QModelIndex()).value<INVLIB::InvEcdSet>();

    // The same fit with MNE-C's defaults (sphere origin (0, 0, 40) mm, EEG sphere radius 90 mm, ...)
    INVLIB::InvDipoleFitSettings settings;
    settings.measname = avePath;
    settings.include_meg = true;
    settings.tmin = 0.09f;
    settings.tmax = 0.1f;
    settings.checkIntegrity();
    INVLIB::InvDipoleFit reference(&settings);
    const INVLIB::InvEcdSet expected = reference.calculateFit();

    QVERIFY(expected.size() > 0);
    QCOMPARE(fit.size(), expected.size());
    for (int i = 0; i < fit.size(); ++i) {
        QVERIFY2((fit[i].rd - expected[i].rd).norm() < 1e-6f,
                 qPrintable(QString("dipole %1 at (%2, %3, %4) mm, MNE-C defaults give (%5, %6, %7) mm")
                                .arg(i)
                                .arg(1e3 * fit[i].rd[0])
                                .arg(1e3 * fit[i].rd[1])
                                .arg(1e3 * fit[i].rd[2])
                                .arg(1e3 * expected[i].rd[0])
                                .arg(1e3 * expected[i].rd[1])
                                .arg(1e3 * expected[i].rd[2])));
        QVERIFY(std::abs(fit[i].good - expected[i].good) < 1e-6);
    }
}

//=============================================================================================================

QTEST_MAIN(TestAnalyzeDipoleFit)
#include "test_analyze_dipolefit.moc"
