#ifndef CONTROL_H
#define CONTROL_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QGroupBox;
QT_END_NAMESPACE

class View;


class Controls : public QWidget
{
    Q_OBJECT

public:
    Controls(View* v, QWidget *parent = 0);

signals:
    void okPressed();
    void quitPressed();

private:
    View* m_view;

    QGroupBox *m_capGroup;
    QGroupBox *m_joinGroup;
    QGroupBox *m_styleGroup;
    QGroupBox *m_pathModeGroup;

    void createCommonControls(QWidget* parent);
    void layoutForDesktop();

private slots:
    void emitQuitSignal();
    void emitOkSignal();

};

#endif // CONTROL_H
