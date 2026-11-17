#include "matrixbutton.h"
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QFontMetrics>
#include <QTextOption>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QEnterEvent>
#include <QMargins>
#endif

namespace
{
constexpr int max_rows = 5;
constexpr int max_cols = 5;
}

DimsChooser::DimsChooser(QWidget * parent, QAction * a)
	:
	QFrame(parent),
	m_column(0),
	m_row(0),
	m_column_w(26.0),
	m_row_h(26.0),
	m_left_margin(10),
	m_top_margin(10),
	m_action(a)
{
	setFrameShadow(Plain);
	setFrameShape(NoFrame);
	setMouseTracking(true);
}

QSize DimsChooser::sizeHint() const
{
	// margins + 5 cells + text banner below
	const int w = m_left_margin * 2 + static_cast<int>(max_cols * m_column_w);
	const int h = m_top_margin * 2 + static_cast<int>(max_rows * m_row_h) + 24;
	return QSize(w, h);
}

void DimsChooser::mouseMoveEvent(QMouseEvent * e)
{
	const QPoint p = e->pos();
	int c = static_cast<int>((p.x() - m_left_margin) / m_column_w);
	int r = static_cast<int>((p.y() - m_top_margin) / m_row_h);
	if (c < 0) c = 0;
	if (c >= max_cols) c = max_cols - 1;
	if (r < 0) r = 0;
	if (r >= max_rows) r = max_rows - 1;

	if (m_column != c || m_row != r)
	{
		m_column = c;
		m_row = r;
		update();
	}
}

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void DimsChooser::enterEvent(QEnterEvent * e)
#else
void DimsChooser::enterEvent(QEvent * e)
#endif
{
	if (m_action) m_action->activate(QAction::Hover);
	QFrame::enterEvent(e);
}

void DimsChooser::leaveEvent(QEvent*)
{
	// Keep previous selection or reset? Reset to 0,0
	update();
}

void DimsChooser::mouseReleaseEvent(QMouseEvent * e)
{
	if (contentsRect().contains(e->pos()))
	{
		emit grid_selected(m_row + 1, m_column + 1);
	}
	QFrame::mouseReleaseEvent(e);
}

void DimsChooser::paintEvent(QPaintEvent * e)
{
	Q_UNUSED(e);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);

	// Background
	painter.fillRect(rect(), QColor(32, 40, 52));

	// Draw grid of cells
	const QColor cellBorder(70, 85, 105);
	const QColor cellEmpty(42, 52, 68);
	const QColor cellHover(28, 120, 212);
	const QColor cellHoverBorder(80, 170, 255);

	const double cellPad = 2.0;

	for (int r = 0; r < max_rows; ++r)
	{
		for (int c = 0; c < max_cols; ++c)
		{
			const double x = m_left_margin + c * m_column_w;
			const double y = m_top_margin + r * m_row_h;
			const QRectF cellRect(x + cellPad, y + cellPad, m_column_w - cellPad * 2, m_row_h - cellPad * 2);

			const bool isSelected = (r <= m_row && c <= m_column);
			painter.fillRect(cellRect, isSelected ? cellHover : cellEmpty);
			painter.setPen(isSelected ? cellHoverBorder : cellBorder);
			painter.drawRect(cellRect);
		}
	}

	// Bottom description banner: "X x Y"
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::white);
	QFont font = painter.font();
	font.setFamily("Segoe UI");
	font.setPointSize(9);
	font.setBold(true);
	painter.setFont(font);

	const int bannerY = m_top_margin + static_cast<int>(max_rows * m_row_h) + 4;
	const QRectF textRect(0, bannerY, width(), 20);
	painter.drawText(textRect, Qt::AlignCenter, QString("%1 x %2").arg(m_row + 1).arg(m_column + 1));
}

DimsChooserAction::DimsChooserAction(QObject * parent)
	: QWidgetAction(parent)
{
	m_widget = new DimsChooser(nullptr, this);
	setDefaultWidget(m_widget);
}

MatrixButton::MatrixButton(float si)
{
	setFocusPolicy(Qt::NoFocus);
	setIconSize(QSize(
		static_cast<int>(si * 18), static_cast<int>(si * 18)));
	setToolButtonStyle(Qt::ToolButtonIconOnly);
	setIcon(QIcon(QString(":/bitmaps/grid.svg")));
	setToolTip(QString("Layout"));
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	p_action = new QAction(
		QIcon(":/bitmaps/grid.svg"),
		QString("Custom"),
		this);
#endif
	p_menu = new QMenu(this);
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	p_menu->addAction(p_action);
	p_menu->addSeparator();
#endif
	DimsChooserAction * chooserAct = new DimsChooserAction(p_menu);
	connect(chooserAct->widget(), &DimsChooser::grid_selected, this, [this](int r, int c) {
		p_menu->hide();
		emit matrix_selected(r, c);
	});
	p_menu->addAction(chooserAct);
	setMenu(p_menu);
	setPopupMode(InstantPopup);
}

void MatrixButton::emit_matrix_selected(int r, int c)
{
	p_menu->hide();
	emit matrix_selected(r, c);
}
