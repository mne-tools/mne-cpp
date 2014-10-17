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

    QString getFilePath() const;

private:
    void scanForProjects();
    void scanForSubjects();

    void updateFileName();

    BabyMEG* m_pBabyMEG;

    Ui::BabyMEGProjectDialog *ui;

    QString m_sBabyMEGDataPath;

    QString m_sCurrentProject;
    QStringList m_sListProjects;

    QString m_sCurrentSubject;
    QStringList m_sListSubjects;
};

} // NAMESPACE

#endif // BABYMEGPROJECTDIALOG_H
