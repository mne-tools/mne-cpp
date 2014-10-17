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

private:
    void addProject();
    void addSubject();

    void paradigmChanged(const QString &newParadigm);

    void scanForProjects();
    void scanForSubjects();

    void selectNewProject(const QString &newProject);
    void selectNewSubject(const QString &newSubject);

    void updateFileName();

    BabyMEG* m_pBabyMEG;

    Ui::BabyMEGProjectDialog *ui;

    QStringList m_sListProjects;
    QStringList m_sListSubjects;
};

} // NAMESPACE

#endif // BABYMEGPROJECTDIALOG_H
