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

//using MNEANONYMIZE::SettingsControllerGui;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setController(SettingsControllerGui*);

//    void loadFile(const QString& fileName);
    SettingsControllerGui* getController() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
//    void open();
//    void save();
//    void about();

private:
//    void createStatusBar();
//    void createAcctions();
//    void saveFile(const QString& fileNme);
    bool confirmClose();

//    QString strippedName(const QString & fullFileName);

   bool m_bDataModified;
   QSharedPointer<Ui::MainWindow> m_pUi;
   SettingsControllerGui* m_pController;
};

}
#endif // MNEANONYMIZE_MAINWINDOW_H
