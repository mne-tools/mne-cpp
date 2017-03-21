#ifndef CONTROL_H
#define CONTROL_H


#include "../disp_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QGroupBox;
QT_END_NAMESPACE

class View;


class DISPSHARED_EXPORT Controls : public QWidget
{
    Q_OBJECT

public:
    Controls(QWidget *parent = Q_NULLPTR);

    Controls(View* v, QWidget *parent = Q_NULLPTR);

    void setView(View* v);

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
