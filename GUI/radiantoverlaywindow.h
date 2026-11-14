#ifndef RADIANTOVERLAYWINDOW_H
#define RADIANTOVERLAYWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QCloseEvent>

class MainWindow;

class RadiantOverlayWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RadiantOverlayWindow(const QString & title, QWidget * contentWidget, MainWindow * parentWindow);
    ~RadiantOverlayWindow() override;

    void set_content(QWidget * contentWidget);
    QWidget * take_content();

protected:
    void closeEvent(QCloseEvent * event) override;

signals:
    void window_closed();

private:
    MainWindow  * mw{nullptr};
    QWidget     * content{nullptr};
    QVBoxLayout * main_layout{nullptr};
};

#endif // RADIANTOVERLAYWINDOW_H
