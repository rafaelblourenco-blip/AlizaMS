#include "studygraphicswidget.h"
#include "studyframewidget.h"
#include <QVBoxLayout>
#include <QToolButton>

StudyFrameWidget::StudyFrameWidget(StudyGraphicsWidget * g)
{
	setupUi(this);
	graphicswidget = g;
	QVBoxLayout * l = new QVBoxLayout(frame);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	l->addWidget(graphicswidget);

	// Close button (X) at top right of the viewport
	close_button = new QToolButton(frame0);
	close_button->setText(QString::fromUtf8("✕"));
	close_button->setToolTip(QString("Fechar esta janela (reagrupar grid)"));
	close_button->setFixedSize(22, 22);
	close_button->setCursor(Qt::PointingHandCursor);
	close_button->setStyleSheet(
		"QToolButton { background-color: rgba(30, 41, 59, 180); color: #94A3B8; border: 1px solid #334155; font-size: 11px; font-weight: bold; border-radius: 3px; margin: 1px 2px; }"
		"QToolButton:hover { background-color: #EF4444; color: #FFFFFF; border: 1px solid #DC2626; }"
		"QToolButton:pressed { background-color: #B91C1C; color: #FFFFFF; border: 1px solid #991B1B; }"
	);
	horizontalLayout_3->addWidget(close_button);

	connect(close_button, &QToolButton::clicked, this, [this]() {
		emit close_requested(this);
	});

	// RadiAnt layout: Sleek vertical scrollbar on the right side of each grid cell
	if (slider)
	{
		if (tools_frame && tools_frame->layout())
		{
			tools_frame->layout()->removeWidget(slider);
			tools_frame->hide();
		}
		slider->setOrientation(Qt::Vertical);
		slider->setFixedWidth(12);
		slider->setInvertedAppearance(true);
		slider->setStyleSheet(
			"QSlider::groove:vertical { background: #0A192F; width: 6px; border-radius: 3px; }"
			"QSlider::sub-page:vertical { background: #0284C7; border-radius: 3px; }"
			"QSlider::handle:vertical { background: #38BDF8; height: 16px; margin: 0 -3px; border-radius: 4px; }"
			"QSlider::handle:vertical:hover { background: #FFFFFF; }");
		horizontalLayout_2->addWidget(slider);
	}
}
