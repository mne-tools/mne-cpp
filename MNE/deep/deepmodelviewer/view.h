#ifndef VIEW_H
#define VIEW_H

#include <QWidget>
#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QLabel;
class QSlider;
class QToolButton;
QT_END_NAMESPACE

class View;

//=============================================================================================================
/**
* GraphicsView visualizes the contents of a QGraphicsScene in a scrollable viewport. To create a scene with
* geometrical items, see QGraphicsScene's documentation.
*
* @brief The GraphicsView class provides a widget for displaying the contents of a QGraphicsScene.
*/
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(View *v) : QGraphicsView(), view(v) { }

protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *) override;
#endif

private:
    View *view;
};


//=============================================================================================================
/**
* The Deep model View Widget containing the graphics view, as well as view related functions
*
* @brief The View Widget containing the graphics view
*/
class View : public QWidget
{
    Q_OBJECT
public:
    explicit View(const QString &name, QWidget *parent = 0);

    QGraphicsView *view() const;

public slots:
    void zoomIn(int level = 1);
    void zoomOut(int level = 1);

#ifndef QT_NO_OPENGL
    void enableOpenGL(bool use_opengl);
    bool usesOpenGL() const { return m_use_opengl; }
#endif
    void enableAntialiasing(bool use_antialiasing);
    bool usesAntialiasing() const { return m_use_antialiasing; }

    void togglePointerMode();
    void print();

private slots:
    void resetView();
    void setResetButtonEnabled();
    void setupMatrix();
    void rotateLeft();
    void rotateRight();

private:
    GraphicsView *graphicsView;
    QToolButton *selectModeButton;
    QToolButton *dragModeButton;
    QToolButton *antialiasButton;
    QToolButton *printButton;
    QToolButton *resetButton;
    QSlider *zoomSlider;
    QSlider *rotateSlider;

    bool m_use_antialiasing;
#ifndef QT_NO_OPENGL
    bool m_use_opengl;
#endif
};

#endif // VIEW_H
