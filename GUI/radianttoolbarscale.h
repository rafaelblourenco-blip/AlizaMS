#ifndef RADIANTTOOLBARSCALE_H
#define RADIANTTOOLBARSCALE_H

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QApplication>
#include <QEvent>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QWindow>

// QScreen geometries are in Qt's logical coordinates.  Do not multiply by
// devicePixelRatio: Qt already scales logical widget sizes on high-DPI screens.
class RadiantToolbarScale : public QObject
{
public:
    RadiantToolbarScale(QWidget *window, QToolBar *toolbar)
        : QObject(toolbar), window_(window), toolbar_(toolbar)
    {
        window->installEventFilter(this);
        schedule();
    }

    static int iconSizeFor(int screenWidth, int screenHeight, int windowWidth)
    {
        if (screenWidth <= 0 || screenHeight <= 0) return 29;
        const int longSide = qMax(screenWidth, screenHeight);
        const int shortSide = qMin(screenWidth, screenHeight);
        // Both dimensions matter: scale proportionally with screen resolution, increased by 20%
        const double scale = qMin(longSide / 1920.0, shortSide / 1080.0);
        // Base size was 24.0 -> increased by 20% is 29.0 (min bound 25, max bound 44)
        const int screenSize = qBound(25, qRound(29.0 * scale), 44);
        const int widthLimit = windowWidth > 0 ? qBound(25, windowWidth / 50, 44) : 44;
        return qMin(screenSize, widthLimit);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == window_ && (event->type() == QEvent::Show ||
            event->type() == QEvent::Resize || event->type() == QEvent::Move ||
            event->type() == QEvent::ScreenChangeInternal)) schedule();
        return QObject::eventFilter(watched, event);
    }

private:
    void schedule()
    {
        if (pending_) return;
        pending_ = true;
        QTimer::singleShot(0, this, [this]() { pending_ = false; update(); });
    }

    void update()
    {
        if (!window_ || !toolbar_) return;
        QWindow *handle = window_->windowHandle();
        if (handle != handle_) {
            if (handle_) QObject::disconnect(handle_, nullptr, this, nullptr);
            handle_ = handle;
            if (handle_) QObject::connect(handle_, &QWindow::screenChanged, this, [this](QScreen *) { schedule(); });
        }
        QScreen *screen = handle ? handle->screen() : QGuiApplication::primaryScreen();
        if (screen != screen_) {
            if (screen_) QObject::disconnect(screen_, nullptr, this, nullptr);
            screen_ = screen;
            if (screen_) {
                QObject::connect(screen_, &QScreen::geometryChanged, this, [this](const QRect &) { schedule(); });
                QObject::connect(screen_, &QScreen::logicalDotsPerInchChanged, this, [this](qreal) { schedule(); });
            }
        }
        const QRect geometry = screen ? screen->availableGeometry() : QRect();
        const int size = iconSizeFor(geometry.width(), geometry.height(), window_->width());
        const QSize icon(size, size);
        if (toolbar_->iconSize() != icon) toolbar_->setIconSize(icon);
        // Menu buttons added as widgets have their own iconSize property.
        const auto buttons = toolbar_->findChildren<QToolButton *>();
        for (QToolButton *button : buttons)
            if (button->iconSize() != icon) button->setIconSize(icon);
    }

    QPointer<QWidget> window_;
    QPointer<QToolBar> toolbar_;
    QPointer<QWindow> handle_;
    QPointer<QScreen> screen_;
    bool pending_{false};
};

#else
#include <QToolBar>
// Qt 4 builds retain their existing fixed-size controls.
class RadiantToolbarScale
{
public:
    RadiantToolbarScale(QWidget *, QToolBar *) {}
};
#endif

#endif
