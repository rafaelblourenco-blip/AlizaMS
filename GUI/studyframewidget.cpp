#include "studygraphicswidget.h"
#include "studyframewidget.h"
#include <QVBoxLayout>

StudyFrameWidget::StudyFrameWidget(StudyGraphicsWidget * g)
{
	setupUi(this);
	graphicswidget = g;
	QVBoxLayout * l = new QVBoxLayout(frame);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	l->addWidget(graphicswidget);

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
