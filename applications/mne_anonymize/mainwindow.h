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

    SettingsControllerGui* getController() const;

    void setLineEditInFile(const QString&);
    void setLineEditOutFile(const QString &f);

    void setCheckBoxBruteMode(bool b);
    void setCheckBoxDeleteInputFileAfter(bool b);
    void setCheckBoxAvoidDeleteConfirmation(bool b);

    void setMeasurementDate(const QString& d);
    void setMeasurementDate(const QDateTime& dt);
    void setCheckBoxMeasurementDateOffset(bool o);
    void setMeasurementDateOffset(int d);

    void setSubjectBirthday(const QString& d);
    void setCheckBoxSubjectBirthdayOffset(bool b);
    void setSubjectBirthdayOffset(int d);

    void setSubjectHis(const QString& h);

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void fileInChanged(const QString& s) const;
    void fileOutChanged(const QString& s) const;
    void useMeasurementOffset(bool f) const;
    void measurementDateChanged(const QDateTime& t) const;
    void measurementDateOffsetChanged(int o) const;
    void birthdayDateChanged(const QDateTime& d) const;
    void useBirthdayOffset(bool f) const;
    void birthdayOffsetChanged(int o) const;
    void subjectHisIdChanged(const QString& text) const;

private slots:

    void on_lineEditInFile_editingFinished();
    void on_lineEditOutFile_editingFinished();

    void on_checkBoxMeasurementDateOffset_stateChanged(int arg1);

    void on_checkBoxBirthdayDateOffset_stateChanged(int arg1);

    void on_dateTimeMeasurementDate_dateTimeChanged(const QDateTime &dateTime);

    void on_spinBoxMeasurementDateOffset_valueChanged(int arg1);

    void on_dateTimeBirthdayDate_dateTimeChanged(const QDateTime &dateTime);

    void on_spinBoxBirthdayDateOffset_valueChanged(int arg1);

    void on_lineEditSubjectHisId_editingFinished();

    void on_toolButton_clicked();

private:

   bool confirmClose();

   bool m_bDataModified;
   bool m_bShowEachField;
   Ui::MainWindow* m_pUi;
   SettingsControllerGui* m_pController;
};

}
#endif // MNEANONYMIZE_MAINWINDOW_H
