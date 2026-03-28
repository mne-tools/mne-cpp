//=============================================================================================================
/**
 * @file     epochwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Dock window that manages reviewed averaging epochs.
 */

#ifndef EPOCHWINDOW_H
#define EPOCHWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Models/epochmodel.h"

#include <QDockWidget>

class QCheckBox;
class QLabel;
class QTableView;
class QToolBar;
class QVBoxLayout;
class QWidget;

namespace MNEBROWSE
{

class MainWindow;

class EpochWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit EpochWindow(QWidget *parent = nullptr);
    ~EpochWindow() override;

    void init();

    EpochModel* getEpochModel() const;

    void setRespectAutoRejects(bool enabled);
    void refreshFromModel();

protected:
    bool event(QEvent *event) override;

private:
    void setupUi();
    void initTable();
    void initToolBar();
    void updateSummary();

private slots:
    void resetManualExclusions();
    void jumpToEpoch(const QModelIndex &current, const QModelIndex &previous);

private:
    MainWindow*  m_pMainWindow = nullptr;
    QWidget*     m_pContents = nullptr;
    QVBoxLayout* m_pLayout = nullptr;
    QLabel*      m_pHintLabel = nullptr;
    QCheckBox*   m_pAutoRejectCheckBox = nullptr;
    QLabel*      m_pSummaryLabel = nullptr;
    QToolBar*    m_pToolBar = nullptr;
    QTableView*  m_pTableView = nullptr;
    EpochModel*  m_pEpochModel = nullptr;
};

} // namespace MNEBROWSE

#endif // EPOCHWINDOW_H
