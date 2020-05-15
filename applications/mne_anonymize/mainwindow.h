#ifndef MNEANONYMIZE_MAINWINDOW_H
#define MNEANONYMIZE_MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

namespace MNEANONYMIZE {
class SettingsControllerGui;
}

namespace MNEANONYMIZE {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SettingsControllerGui* controller = nullptr);

    ~MainWindow() override;

    void setController(SettingsControllerGui*);

//    void loadFile(const QString& fileName);
    SettingsControllerGui* getController() const;

    void setLineEditInFile(const QString&);
    void setLineEditOutFile(const QString &f);
    void setBruteMode(bool b);
    void setMeasurementDate(const QString& d);
    void setMeasurementDateOffset(int d);
    void setSubjectBirthday(const QString& d);
    void setSubjectBirthdayOffset(int d);
    void setSubjectHis(const QString& h);

protected:
    void closeEvent(QCloseEvent *event) override;
signals:
    void fileInChanged(const QString& s) const;
    void fileOutChanged(const QString& s) const;

private slots:
//    void open();
//    void save();
//    void about();
    void on_lineEditInFile_editingFinished();
    void on_lineEditOutFile_editingFinished();

    void on_checkBoxMeasurementDate_stateChanged(int arg1);
    void on_checkBoxMeasurementDateOffset_stateChanged(int arg1);

    void on_checkBoxBirthdayDate_stateChanged(int arg1);
    void on_checkBoxBirthdayDateOffset_stateChanged(int arg1);

    void on_checkBoxHisValue_clicked(bool checked);





private:
//    void createStatusBar();
//    void createAcctions();
//    void saveFile(const QString& fileNme);
    bool confirmClose();

//    QString strippedName(const QString & fullFileName);

   bool m_bDataModified;
   Ui::MainWindow* m_pUi;
   SettingsControllerGui* m_pController;
};

}
#endif // MNEANONYMIZE_MAINWINDOW_H
