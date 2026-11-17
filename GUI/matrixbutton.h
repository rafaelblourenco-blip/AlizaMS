#ifndef A_MATRIXBUTTON_H
#define A_MATRIXBUTTON_H

#include <QWidget>
#include <QToolButton>
#include <QAction>
#include <QWidgetAction>
#include <QFrame>

#define MATRIX_BUTTON_CUSTOM_ACT 0

class QMenu;
class MatrixButton;

class DimsChooser : public QFrame
{
    Q_OBJECT
public:
    DimsChooser(QWidget * parent = nullptr, QAction * action = nullptr);
    QSize sizeHint() const override;
    void mouseMoveEvent(QMouseEvent*) override;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    void enterEvent(QEnterEvent*) override;
#else
    void enterEvent(QEvent*) override;
#endif
    void leaveEvent(QEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;

signals:
    void grid_selected(int rows, int cols);

private:
    int      m_column{0};
    int      m_row{0};
    double   m_column_w{26.0};
    double   m_row_h{26.0};
    int      m_left_margin{8};
    int      m_top_margin{8};
    QAction *m_action{nullptr};
};

class DimsChooserAction : public QWidgetAction
{
    Q_OBJECT
public:
    DimsChooserAction(QObject * parent = nullptr);
    DimsChooser * widget() const { return m_widget; }

private:
    DimsChooser * m_widget;
};

class MatrixButton : public QToolButton
{
Q_OBJECT
public:
	MatrixButton(float);
	void emit_matrix_selected(int, int);
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	QAction * p_action;
#endif

signals:
	void matrix_selected(int, int);

private:
	QMenu * p_menu;
};

#endif
