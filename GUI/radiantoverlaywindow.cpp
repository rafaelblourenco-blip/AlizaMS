#include "radiantoverlaywindow.h"
#include "mainwindow.h"
#include <QVBoxLayout>

RadiantOverlayWindow::RadiantOverlayWindow(const QString & title, QWidget * contentWidget, MainWindow * parentWindow)
    : QWidget(parentWindow, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
      mw(parentWindow), content(contentWidget)
{
    setWindowTitle(title);
    resize(960, 680);
    setStyleSheet(
        "RadiantOverlayWindow { background-color: #071728; border: 2px solid #1E3A5F; }"
        "QWidget { background-color: #000000; color: #E2E8F0; }");

    main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(2);

    if (content)
    {
        main_layout->addWidget(content);
        content->show();
    }
}

RadiantOverlayWindow::~RadiantOverlayWindow()
{
}

void RadiantOverlayWindow::set_content(QWidget * contentWidget)
{
    if (content && content != contentWidget)
    {
        main_layout->removeWidget(content);
    }
    content = contentWidget;
    if (content)
    {
        main_layout->addWidget(content);
        content->show();
    }
}

QWidget * RadiantOverlayWindow::take_content()
{
    QWidget * ret = content;
    if (content)
    {
        main_layout->removeWidget(content);
        content = nullptr;
    }
    return ret;
}

void RadiantOverlayWindow::closeEvent(QCloseEvent * event)
{
    emit window_closed();
    QWidget::closeEvent(event);
}
