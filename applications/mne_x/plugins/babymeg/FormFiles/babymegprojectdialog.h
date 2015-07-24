#ifndef BABYMEGPROJECTDIALOG_H
#define BABYMEGPROJECTDIALOG_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class BabyMEGProjectDialog;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BabyMEGPlugin
//=============================================================================================================

namespace BabyMEGPlugin
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEG;


class BabyMEGProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BabyMEGProjectDialog(BabyMEG* p_pBabyMEG, QWidget *parent = 0);
    ~BabyMEGProjectDialog();

    void setRecordingElapsedTime(int secs);

signals:
    void timerChanged(const QTime & time);
    void recordingTimerStateChanged(bool state);

private:
    void addProject();
    void addSubject();

    void deleteProject();
    void deleteSubject();

    void paradigmChanged(const QString &newParadigm);

    void scanForProjects();
    void scanForSubjects();

    void selectNewProject(const QString &newProject);
    void selectNewSubject(const QString &newSubject);

    void updateFileName();

    void onTimeChanged(const QTime & time);
    void onRecordingTimerStateChanged(bool state);

    BabyMEG* m_pBabyMEG;

    Ui::BabyMEGProjectDialog *ui;

    QStringList m_sListProjects;
    QStringList m_sListSubjects;
};

} // NAMESPACE

#endif // BABYMEGPROJECTDIALOG_H
