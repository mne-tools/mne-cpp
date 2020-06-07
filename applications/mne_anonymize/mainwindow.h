#ifndef MNEANONYMIZE_MAINWINDOW_H
#define MNEANONYMIZE_MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>

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

public slots:
    void setIdFileVersion(double v);
    void setIdMeasurementDate(QDateTime d);
    void setIdMacAddress(QString mac);
    void setFileMeasurementDate(QDateTime d);
    void setFileComment(QString c);
    void setFileExperimenter(QString e);

    void setSubjectId(int i);
    void setSubjectFirstName(QString fn);
    void setSubjectMiddleName(QString mn);
    void setSubjectLastName(QString ln);
    void setSubjectBirthday(QDateTime b);
    void setSubjectSex(int s);
    void setSubjectHand(int h);
    void setSubjectWeight(float w);
    void setSubjectHeight(float h);
    void setSubjectComment(QString c);
    void setSubjectHisId(QString);
    void setProjectId(int);
    void setProjectName(QString);
    void setProjectAim(QString);
    void setProjectPersons(QString);
    void setProjectComment(QString);

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void fileInChanged(const QString& s) const;
    void fileOutChanged(const QString& s) const;
    void measurementDateChanged(const QDateTime& t) const;
    void useMeasurementOffset(bool f) const;
    void measurementDateOffsetChanged(int o) const;
    void birthdayDateChanged(const QDateTime& d) const;
    void useBirthdayOffset(bool f) const;
    void birthdayOffsetChanged(int o) const;
    void subjectHisIdChanged(const QString& text) const;

//    crear mas signals para cada caja

private slots:

    void checkBoxMeasurementDateOffsetStateChanged(int arg1);
    void checkBoxBirthdayDateOffsetStateChanged(int arg1);

    //estos se pueden eliminar al connect signal to signal
    void lineEditInFileEditingFinished(); //
    void lineEditOutFileEditingFinished();//

    void dateTimeMeasurementDateDateTimeChanged(const QDateTime &dateTime);
    void spinBoxMeasurementDateOffsetValueChanged(int arg1);
    void dateTimeBirthdayDateDateTimeChanged(const QDateTime &dateTime);
    void spinBoxBirthdayDateOffsetValueChanged(int arg1);
    void lineEditSubjectHisIdEditingFinished();

//    completar signal to signal

    void seeExtraInformationClicked();

    void helpButtonClicked();

    void setExtraObjectstoState();

    void setCheckBoxEditExtraUnmutable();

private:

    bool confirmClose();

    void setDefautlStateUi();
    void setupConections();

    bool m_bIdFileVersionFound;
    bool m_bIdMeasurementDateFound;
    bool m_bIdMacAddressFound;
    bool m_bFileMeasurementDateFound;
    bool m_bFileExperimenterFound;
    bool m_bFileCommentFound;

    bool m_bSubjectIdFound;
    bool m_bSubjectFirstNameFound;
    bool m_bSubjectMiddleNameFound;
    bool m_bSubjectLastNameFound;
    bool m_bSubjectBirthdayFound;
    bool m_bSubjectSexFound;
    bool m_bSubjectHandFound;
    bool m_bSubjectWeightFound;
    bool m_bSubjectHeightFound;
    bool m_bSubjectCommentFound;
    bool m_bSubjectHisIdFound;

    bool m_bProjectIdFound;
    bool m_bProjectAimFound;
    bool m_bProjectNameFound;
    bool m_bProjectPersonsFound;
    bool m_bProjectCommentFound;

    bool m_bHideEachField;
    Ui::MainWindow* m_pUi;
    SettingsControllerGui* m_pController;
};

}
#endif // MNEANONYMIZE_MAINWINDOW_H
