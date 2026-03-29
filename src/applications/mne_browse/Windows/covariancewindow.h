//=============================================================================================================
/**
 * @file     covariancewindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @version  2.1.0
 * @date     March, 2026
 *
 * @brief    Dock window that inspects covariance data and controls whitening.
 */

#ifndef COVARIANCEWINDOW_H
#define COVARIANCEWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/types.h"

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <QDockWidget>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QTextEdit;
class QToolBar;
class QVBoxLayout;
class QWidget;

namespace MNEBROWSE
{

class CovarianceWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit CovarianceWindow(QWidget *parent = nullptr);
    ~CovarianceWindow() override;

    void init();

    void setCovariance(const FIFFLIB::FiffCov& covariance,
                       const QString& sourceDescription = QString(),
                       FIFFLIB::FiffInfo::SPtr fiffInfo = FIFFLIB::FiffInfo::SPtr());
    void clearCovariance();

    void setWhiteningSettings(const WhiteningSettings& settings);
    WhiteningSettings whiteningSettings() const;

private:
    void setupUi();
    void initControls();
    void updateSummary();
    void updateToggleState();
    void emitSettingsChanged();

private slots:
    void resetDefaults();

signals:
    void whiteningSettingsChanged(const MNEBROWSE::WhiteningSettings& settings);

private:
    QWidget*         m_pContents = nullptr;
    QVBoxLayout*     m_pLayout = nullptr;
    QLabel*          m_pHintLabel = nullptr;
    QTextEdit*       m_pSummaryTextEdit = nullptr;
    QToolBar*        m_pToolBar = nullptr;
    QCheckBox*       m_pWhitenButterflyCheckBox = nullptr;
    QCheckBox*       m_pWhitenLayoutCheckBox = nullptr;
    QCheckBox*       m_pUseProjCheckBox = nullptr;
    QDoubleSpinBox*  m_pRegMagSpinBox = nullptr;
    QDoubleSpinBox*  m_pRegGradSpinBox = nullptr;
    QDoubleSpinBox*  m_pRegEegSpinBox = nullptr;

    FIFFLIB::FiffCov        m_covariance;
    FIFFLIB::FiffInfo::SPtr m_pFiffInfo;
    QString                 m_sSourceDescription;
};

} // namespace MNEBROWSE

#endif // COVARIANCEWINDOW_H
