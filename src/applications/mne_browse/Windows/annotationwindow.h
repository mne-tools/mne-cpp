//=============================================================================================================
/**
 * @file     annotationwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @version  2.1.0
 * @date     March, 2026
 *
 * @brief    Dock window that manages browser annotations.
 */

#ifndef ANNOTATIONWINDOW_H
#define ANNOTATIONWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Models/annotationmodel.h"

#include <QDockWidget>
#include <QSet>

class QLabel;
class QTableView;
class QToolBar;
class QVBoxLayout;

namespace MNEBROWSE
{

class MainWindow;

class AnnotationWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit AnnotationWindow(QWidget *parent = nullptr);
    ~AnnotationWindow() override;

    void init();

    AnnotationModel* getAnnotationModel() const;
    QTableView* getAnnotationTableView() const;
    void addAnnotation(int startSample, int endSample, const QString& label);
    bool isDescriptionVisible(const QString& description) const;

signals:
    void visibilityFilterChanged();

protected:
    bool event(QEvent *event) override;

private:
    void setupUi();
    void initTable();
    void initToolBar();

private slots:
    void removeSelectedAnnotations();
    void jumpToAnnotation(const QModelIndex &current, const QModelIndex &previous);
    void selectVisibleDescriptions();

private:
    MainWindow*      m_pMainWindow = nullptr;
    QWidget*         m_pContents = nullptr;
    QVBoxLayout*     m_pLayout = nullptr;
    QLabel*          m_pHintLabel = nullptr;
    QToolBar*        m_pToolBar = nullptr;
    QTableView*      m_pTableView = nullptr;
    AnnotationModel* m_pAnnotationModel = nullptr;
    QSet<QString>    m_hiddenDescriptions;
};

} // namespace MNEBROWSE

#endif // ANNOTATIONWINDOW_H
