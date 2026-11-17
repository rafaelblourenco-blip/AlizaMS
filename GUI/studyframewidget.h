#ifndef A_STUDYFRAMEWIDGET_H
#define A_STUDYFRAMEWIDGET_H

#include "ui_studyframewidget.h"
#include <QToolButton>

class StudyGraphicsWidget;

class StudyFrameWidget : public QWidget, public Ui::StudyFrameWidget
{

Q_OBJECT

public:
	StudyFrameWidget(StudyGraphicsWidget*);
	~StudyFrameWidget() = default;
	StudyGraphicsWidget * graphicswidget;
	QToolButton * close_button{nullptr};

signals:
	void close_requested(StudyFrameWidget * frame);
};

#endif

