//=============================================================================================================
/**
 * @file     covariancewindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @brief    Definition of the CovarianceWindow class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariancewindow.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <limits>

using namespace MNEBROWSE;

namespace
{

QString summarizeChannelNames(const QStringList& names, int limit = 8)
{
    if(names.isEmpty()) {
        return QStringLiteral("none");
    }

    if(names.size() <= limit) {
        return names.join(QStringLiteral(", "));
    }

    return QStringLiteral("%1, ... (%2 total)")
        .arg(names.mid(0, limit).join(QStringLiteral(", ")))
        .arg(names.size());
}

QString covarianceSummary(const FIFFLIB::FiffCov& covariance,
                          const QString& sourceDescription,
                          const FIFFLIB::FiffInfo::SPtr& fiffInfo)
{
    if(covariance.isEmpty()) {
        return QStringLiteral("No covariance is currently loaded.\n\nCompute or load a covariance matrix to inspect it here and drive whitening from this manager.");
    }

    int magCount = 0;
    int gradCount = 0;
    int eegCount = 0;
    int otherCount = 0;

    if(fiffInfo) {
        for(const QString& channelName : covariance.names) {
            const int channelIndex = fiffInfo->ch_names.indexOf(channelName);
            if(channelIndex < 0 || channelIndex >= fiffInfo->chs.size()) {
                ++otherCount;
                continue;
            }

            const auto& channelInfo = fiffInfo->chs.at(channelIndex);
            if(channelInfo.kind == FIFFV_EEG_CH) {
                ++eegCount;
            } else if(channelInfo.kind == FIFFV_MEG_CH && channelInfo.unit == FIFF_UNIT_T) {
                ++magCount;
            } else if(channelInfo.kind == FIFFV_MEG_CH && channelInfo.unit == FIFF_UNIT_T_M) {
                ++gradCount;
            } else {
                ++otherCount;
            }
        }
    }

    QStringList lines;
    if(!sourceDescription.isEmpty()) {
        lines << QStringLiteral("Source: %1").arg(sourceDescription);
    }
    lines << QStringLiteral("Storage: %1 covariance").arg(covariance.diag ? QStringLiteral("diagonal")
                                                                          : QStringLiteral("full"));
    lines << QStringLiteral("Dimensions: %1 x %1").arg(covariance.dim);
    lines << QStringLiteral("Degrees of freedom: %1").arg(covariance.nfree);
    lines << QStringLiteral("Channels: %1").arg(covariance.names.size());
    if(fiffInfo) {
        lines << QStringLiteral("Types: %1 grad, %2 mag, %3 EEG, %4 other")
                     .arg(gradCount)
                     .arg(magCount)
                     .arg(eegCount)
                     .arg(otherCount);
    }
    lines << QStringLiteral("Projectors: %1").arg(covariance.projs.size());
    lines << QStringLiteral("Bad channels: %1").arg(summarizeChannelNames(covariance.bads, 6));
    lines << QStringLiteral("Channel preview: %1").arg(summarizeChannelNames(covariance.names, 10));
    lines << QStringLiteral("Eigen decomposition: %1").arg((covariance.eig.size() > 0 && covariance.eigvec.size() > 0)
                                                           ? QStringLiteral("available")
                                                           : QStringLiteral("not stored"));

    return lines.join(QLatin1Char('\n'));
}

} // namespace

//=============================================================================================================

CovarianceWindow::CovarianceWindow(QWidget *parent)
: QDockWidget(parent)
{
    setupUi();
}

//=============================================================================================================

CovarianceWindow::~CovarianceWindow() = default;

//=============================================================================================================

void CovarianceWindow::init()
{
    initControls();
    updateSummary();
    updateToggleState();
}

//=============================================================================================================

void CovarianceWindow::setCovariance(const FIFFLIB::FiffCov& covariance,
                                     const QString& sourceDescription,
                                     FIFFLIB::FiffInfo::SPtr fiffInfo)
{
    m_covariance = covariance;
    m_sSourceDescription = sourceDescription;
    m_pFiffInfo = fiffInfo;

    updateSummary();
    updateToggleState();
}

//=============================================================================================================

void CovarianceWindow::clearCovariance()
{
    m_covariance.clear();
    m_sSourceDescription.clear();
    m_pFiffInfo.clear();

    updateSummary();
    updateToggleState();
}

//=============================================================================================================

void CovarianceWindow::setWhiteningSettings(const WhiteningSettings& settings)
{
    QSignalBlocker blockButterfly(m_pWhitenButterflyCheckBox);
    QSignalBlocker blockLayout(m_pWhitenLayoutCheckBox);
    QSignalBlocker blockProj(m_pUseProjCheckBox);
    QSignalBlocker blockMag(m_pRegMagSpinBox);
    QSignalBlocker blockGrad(m_pRegGradSpinBox);
    QSignalBlocker blockEeg(m_pRegEegSpinBox);

    m_pWhitenButterflyCheckBox->setChecked(settings.enableButterfly);
    m_pWhitenLayoutCheckBox->setChecked(settings.enableLayout);
    m_pUseProjCheckBox->setChecked(settings.useProj);
    m_pRegMagSpinBox->setValue(settings.regMag);
    m_pRegGradSpinBox->setValue(settings.regGrad);
    m_pRegEegSpinBox->setValue(settings.regEeg);
}

//=============================================================================================================

WhiteningSettings CovarianceWindow::whiteningSettings() const
{
    WhiteningSettings settings;
    settings.enableButterfly = m_pWhitenButterflyCheckBox->isChecked();
    settings.enableLayout = m_pWhitenLayoutCheckBox->isChecked();
    settings.useProj = m_pUseProjCheckBox->isChecked();
    settings.regMag = m_pRegMagSpinBox->value();
    settings.regGrad = m_pRegGradSpinBox->value();
    settings.regEeg = m_pRegEegSpinBox->value();
    return settings;
}

//=============================================================================================================

void CovarianceWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Covariance Manager"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    m_pContents = new QWidget(this);
    m_pLayout = new QVBoxLayout(m_pContents);
    m_pLayout->setContentsMargins(6, 6, 6, 6);
    m_pLayout->setSpacing(6);

    m_pHintLabel = new QLabel(QStringLiteral("Inspect the active covariance matrix and tune whitening without leaving the browser."), m_pContents);
    m_pHintLabel->setWordWrap(true);
    m_pLayout->addWidget(m_pHintLabel);

    m_pSummaryTextEdit = new QTextEdit(m_pContents);
    m_pSummaryTextEdit->setReadOnly(true);
    m_pSummaryTextEdit->setMinimumHeight(160);
    m_pLayout->addWidget(m_pSummaryTextEdit);

    // Heatmap visualization
    QGroupBox* heatmapGroup = new QGroupBox(QStringLiteral("Covariance Matrix"), m_pContents);
    QVBoxLayout* heatmapLayout = new QVBoxLayout(heatmapGroup);

    m_pHeatmapChannelType = new QComboBox(heatmapGroup);
    m_pHeatmapChannelType->addItems({QStringLiteral("All"), QStringLiteral("MEG_grad"),
                                     QStringLiteral("MEG_mag"), QStringLiteral("EEG")});
    heatmapLayout->addWidget(m_pHeatmapChannelType);

    m_pHeatmapLabel = new QLabel(heatmapGroup);
    m_pHeatmapLabel->setAlignment(Qt::AlignCenter);
    m_pHeatmapLabel->setMinimumHeight(200);
    m_pHeatmapLabel->setText(QStringLiteral("No covariance loaded."));
    heatmapLayout->addWidget(m_pHeatmapLabel);

    m_pLayout->addWidget(heatmapGroup);

    QGroupBox* whiteningGroup = new QGroupBox(QStringLiteral("Whitening"), m_pContents);
    QFormLayout* whiteningLayout = new QFormLayout(whiteningGroup);

    m_pWhitenButterflyCheckBox = new QCheckBox(QStringLiteral("Whiten butterfly plot"), whiteningGroup);
    whiteningLayout->addRow(m_pWhitenButterflyCheckBox);

    m_pWhitenLayoutCheckBox = new QCheckBox(QStringLiteral("Whiten layout plot"), whiteningGroup);
    whiteningLayout->addRow(m_pWhitenLayoutCheckBox);

    m_pUseProjCheckBox = new QCheckBox(QStringLiteral("Apply projectors during regularization"), whiteningGroup);
    m_pUseProjCheckBox->setChecked(true);
    whiteningLayout->addRow(m_pUseProjCheckBox);

    m_pRegMagSpinBox = new QDoubleSpinBox(whiteningGroup);
    m_pRegMagSpinBox->setDecimals(3);
    m_pRegMagSpinBox->setRange(0.0, 5.0);
    m_pRegMagSpinBox->setSingleStep(0.01);
    m_pRegMagSpinBox->setValue(0.1);
    whiteningLayout->addRow(QStringLiteral("Mag regularization"), m_pRegMagSpinBox);

    m_pRegGradSpinBox = new QDoubleSpinBox(whiteningGroup);
    m_pRegGradSpinBox->setDecimals(3);
    m_pRegGradSpinBox->setRange(0.0, 5.0);
    m_pRegGradSpinBox->setSingleStep(0.01);
    m_pRegGradSpinBox->setValue(0.1);
    whiteningLayout->addRow(QStringLiteral("Grad regularization"), m_pRegGradSpinBox);

    m_pRegEegSpinBox = new QDoubleSpinBox(whiteningGroup);
    m_pRegEegSpinBox->setDecimals(3);
    m_pRegEegSpinBox->setRange(0.0, 5.0);
    m_pRegEegSpinBox->setSingleStep(0.01);
    m_pRegEegSpinBox->setValue(0.1);
    whiteningLayout->addRow(QStringLiteral("EEG regularization"), m_pRegEegSpinBox);

    m_pLayout->addWidget(whiteningGroup);
    setWidget(m_pContents);
}

//=============================================================================================================

void CovarianceWindow::initControls()
{
    m_pToolBar = new QToolBar(this);
    m_pToolBar->setMovable(false);

    QAction* resetDefaultsAction = new QAction(tr("Reset whitening defaults"), this);
    connect(resetDefaultsAction, &QAction::triggered,
            this, &CovarianceWindow::resetDefaults);
    m_pToolBar->addAction(resetDefaultsAction);

    m_pLayout->insertWidget(2, m_pToolBar);

    connect(m_pWhitenButterflyCheckBox, &QCheckBox::toggled,
            this, [this]() { emitSettingsChanged(); });
    connect(m_pWhitenLayoutCheckBox, &QCheckBox::toggled,
            this, [this]() { emitSettingsChanged(); });
    connect(m_pUseProjCheckBox, &QCheckBox::toggled,
            this, [this]() { emitSettingsChanged(); });
    connect(m_pRegMagSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { emitSettingsChanged(); });
    connect(m_pRegGradSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { emitSettingsChanged(); });
    connect(m_pRegEegSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { emitSettingsChanged(); });

    connect(m_pHeatmapChannelType, &QComboBox::currentTextChanged,
            this, [this]() { updateHeatmap(); });
}

//=============================================================================================================

void CovarianceWindow::updateSummary()
{
    if(!m_pSummaryTextEdit) {
        return;
    }

    m_pSummaryTextEdit->setPlainText(covarianceSummary(m_covariance,
                                                       m_sSourceDescription,
                                                       m_pFiffInfo));
    updateHeatmap();
}

//=============================================================================================================

void CovarianceWindow::updateHeatmap()
{
    if(!m_pHeatmapLabel)
        return;

    if(m_covariance.isEmpty() || m_covariance.dim <= 0) {
        m_pHeatmapLabel->setPixmap(QPixmap());
        m_pHeatmapLabel->setText(QStringLiteral("No covariance loaded."));
        return;
    }

    // Determine which channel indices to include based on the filter
    QList<int> indices;
    const QString filter = m_pHeatmapChannelType ? m_pHeatmapChannelType->currentText()
                                                 : QStringLiteral("All");

    for(int i = 0; i < m_covariance.names.size(); ++i) {
        if(filter == QStringLiteral("All")) {
            indices.append(i);
            continue;
        }

        if(m_pFiffInfo) {
            const int chIdx = m_pFiffInfo->ch_names.indexOf(m_covariance.names[i]);
            if(chIdx < 0 || chIdx >= m_pFiffInfo->chs.size())
                continue;

            const auto& chInfo = m_pFiffInfo->chs.at(chIdx);
            if(filter == QStringLiteral("MEG_grad")
               && chInfo.kind == FIFFV_MEG_CH && chInfo.unit == FIFF_UNIT_T_M) {
                indices.append(i);
            } else if(filter == QStringLiteral("MEG_mag")
                      && chInfo.kind == FIFFV_MEG_CH && chInfo.unit == FIFF_UNIT_T) {
                indices.append(i);
            } else if(filter == QStringLiteral("EEG") && chInfo.kind == FIFFV_EEG_CH) {
                indices.append(i);
            }
        } else {
            // Without FiffInfo, include all
            indices.append(i);
        }
    }

    if(indices.isEmpty()) {
        m_pHeatmapLabel->setPixmap(QPixmap());
        m_pHeatmapLabel->setText(QStringLiteral("No channels of this type in covariance."));
        return;
    }

    const int n = indices.size();

    // Extract the sub-matrix (or diagonal) and find min/max for color mapping
    double vMin = std::numeric_limits<double>::max();
    double vMax = std::numeric_limits<double>::lowest();

    // Build the values to display
    Eigen::MatrixXd subMatrix(n, n);

    if(m_covariance.diag) {
        subMatrix.setZero();
        for(int i = 0; i < n; ++i) {
            double val = m_covariance.data(indices[i], 0);
            subMatrix(i, i) = val;
            if(val < vMin) vMin = val;
            if(val > vMax) vMax = val;
        }
    } else {
        for(int i = 0; i < n; ++i) {
            for(int j = 0; j < n; ++j) {
                double val = m_covariance.data(indices[i], indices[j]);
                subMatrix(i, j) = val;
                if(val < vMin) vMin = val;
                if(val > vMax) vMax = val;
            }
        }
    }

    // Use symmetric color range around zero
    const double absMax = qMax(qAbs(vMin), qAbs(vMax));
    if(absMax < 1e-30) {
        m_pHeatmapLabel->setPixmap(QPixmap());
        m_pHeatmapLabel->setText(QStringLiteral("Covariance matrix is effectively zero."));
        return;
    }

    // Create the heatmap image
    const int imgSize = qMin(n, 512); // cap image size
    QImage img(imgSize, imgSize, QImage::Format_RGB32);

    for(int py = 0; py < imgSize; ++py) {
        const int row = py * n / imgSize;
        QRgb* scanLine = reinterpret_cast<QRgb*>(img.scanLine(py));
        for(int px = 0; px < imgSize; ++px) {
            const int col = px * n / imgSize;
            const double val = subMatrix(row, col);

            // Blue-white-red diverging colormap
            const double t = val / absMax; // in [-1, 1]
            int r, g, b;
            if(t >= 0) {
                // White to red
                r = 255;
                g = static_cast<int>(255.0 * (1.0 - t));
                b = static_cast<int>(255.0 * (1.0 - t));
            } else {
                // White to blue
                r = static_cast<int>(255.0 * (1.0 + t));
                g = static_cast<int>(255.0 * (1.0 + t));
                b = 255;
            }
            scanLine[px] = qRgb(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
        }
    }

    // Scale to fit the label width
    const int displaySize = qMin(m_pHeatmapLabel->width() - 10, 400);
    QPixmap pixmap = QPixmap::fromImage(img).scaled(displaySize, displaySize,
                                                     Qt::KeepAspectRatio,
                                                     Qt::FastTransformation);
    m_pHeatmapLabel->setPixmap(pixmap);
    m_pHeatmapLabel->setText(QString());
}

//=============================================================================================================

void CovarianceWindow::updateToggleState()
{
    const bool hasCovariance = !m_covariance.isEmpty();
    m_pWhitenButterflyCheckBox->setEnabled(hasCovariance);
    m_pWhitenLayoutCheckBox->setEnabled(hasCovariance);
}

//=============================================================================================================

void CovarianceWindow::emitSettingsChanged()
{
    emit whiteningSettingsChanged(whiteningSettings());
}

//=============================================================================================================

void CovarianceWindow::resetDefaults()
{
    WhiteningSettings settings = whiteningSettings();
    settings.regMag = 0.1;
    settings.regGrad = 0.1;
    settings.regEeg = 0.1;
    settings.useProj = true;

    setWhiteningSettings(settings);
    emitSettingsChanged();
}
