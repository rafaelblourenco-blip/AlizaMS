#ifndef RADIANTFILTERMANAGER_H
#define RADIANTFILTERMANAGER_H

#include <QObject>
#include <QImage>

enum class RadiantFilterType {
    None = 0,
    Sharpen1,
    Sharpen2,
    Sharpen3,
    Smooth1,
    Smooth2,
    Smooth3,
    Edge1,
    Edge2,
    Edge3,
    EmbossN,
    EmbossW,
    EmbossD,
    Emboss1,
    Emboss2
};

class RadiantFilterManager : public QObject
{
    Q_OBJECT
public:
    static RadiantFilterManager * instance();

    void set_filter(RadiantFilterType type);
    RadiantFilterType get_filter() const { return current_filter; }

    void next_filter();
    void prev_filter();
    void next_sharpen_filter();

    // Fast 3x3 convolution filter applied to QImage
    static QImage apply_convolution(const QImage & src, RadiantFilterType type);

signals:
    void filter_changed(RadiantFilterType type);

private:
    explicit RadiantFilterManager(QObject * parent = nullptr);
    RadiantFilterType current_filter{RadiantFilterType::None};
};

#endif // RADIANTFILTERMANAGER_H
