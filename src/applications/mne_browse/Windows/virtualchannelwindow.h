//=============================================================================================================
/**
 * @file     virtualchannelwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Dock window that manages virtual bipolar channels.
 */

#ifndef VIRTUALCHANNELWINDOW_H
#define VIRTUALCHANNELWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Models/virtualchannelmodel.h"

#include <QDockWidget>

class QLabel;
class QTableView;
class QToolBar;
class QVBoxLayout;

namespace MNEBROWSE
{

class MainWindow;

class VirtualChannelWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit VirtualChannelWindow(QWidget *parent = nullptr);
    ~VirtualChannelWindow() override;

    void init();

    VirtualChannelModel* getVirtualChannelModel() const;
    QTableView* getVirtualChannelTableView() const;
    void setAvailableChannelNames(const QStringList& channelNames);

protected:
    bool event(QEvent *event) override;

private:
    void setupUi();
    void initTable();
    void initToolBar();
    bool promptForVirtualChannel(QString& name,
                                 QString& positiveChannel,
                                 QString& negativeChannel) const;

private slots:
    void addVirtualChannel();
    void removeSelectedVirtualChannels();

private:
    MainWindow*          m_pMainWindow = nullptr;
    QWidget*             m_pContents = nullptr;
    QVBoxLayout*         m_pLayout = nullptr;
    QLabel*              m_pHintLabel = nullptr;
    QToolBar*            m_pToolBar = nullptr;
    QTableView*          m_pTableView = nullptr;
    VirtualChannelModel* m_pVirtualChannelModel = nullptr;
    QStringList          m_availableChannelNames;
};

} // namespace MNEBROWSE

#endif // VIRTUALCHANNELWINDOW_H
