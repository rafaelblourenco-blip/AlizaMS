#include "radiantfiltermanager.h"
#include <algorithm>
#include <vector>

RadiantFilterManager::RadiantFilterManager(QObject * parent)
    : QObject(parent)
{
}

RadiantFilterManager * RadiantFilterManager::instance()
{
    static RadiantFilterManager inst;
    return &inst;
}

void RadiantFilterManager::set_filter(RadiantFilterType type)
{
    if (current_filter != type)
    {
        current_filter = type;
        emit filter_changed(type);
    }
}

void RadiantFilterManager::next_filter()
{
    int next = static_cast<int>(current_filter) + 1;
    if (next > static_cast<int>(RadiantFilterType::Emboss2)) next = 0;
    set_filter(static_cast<RadiantFilterType>(next));
}

void RadiantFilterManager::prev_filter()
{
    int prev = static_cast<int>(current_filter) - 1;
    if (prev < 0) prev = static_cast<int>(RadiantFilterType::Emboss2);
    set_filter(static_cast<RadiantFilterType>(prev));
}

void RadiantFilterManager::next_sharpen_filter()
{
    if (current_filter == RadiantFilterType::Sharpen1) set_filter(RadiantFilterType::Sharpen2);
    else if (current_filter == RadiantFilterType::Sharpen2) set_filter(RadiantFilterType::Sharpen3);
    else set_filter(RadiantFilterType::Sharpen1);
}

QImage RadiantFilterManager::apply_convolution(const QImage & src, RadiantFilterType type)
{
    if (type == RadiantFilterType::None || src.isNull()) return src;

    float kernel[3][3];
    float divisor = 1.0f;
    float bias = 0.0f;

    switch (type)
    {
    case RadiantFilterType::Sharpen1:
        kernel[0][0] =  0; kernel[0][1] = -1; kernel[0][2] =  0;
        kernel[1][0] = -1; kernel[1][1] =  5; kernel[1][2] = -1;
        kernel[2][0] =  0; kernel[2][1] = -1; kernel[2][2] =  0;
        break;
    case RadiantFilterType::Sharpen2:
        kernel[0][0] = -1; kernel[0][1] = -1; kernel[0][2] = -1;
        kernel[1][0] = -1; kernel[1][1] =  9; kernel[1][2] = -1;
        kernel[2][0] = -1; kernel[2][1] = -1; kernel[2][2] = -1;
        break;
    case RadiantFilterType::Sharpen3:
        kernel[0][0] = -1; kernel[0][1] = -2; kernel[0][2] = -1;
        kernel[1][0] = -2; kernel[1][1] = 13; kernel[1][2] = -2;
        kernel[2][0] = -1; kernel[2][1] = -2; kernel[2][2] = -1;
        break;
    case RadiantFilterType::Smooth1:
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                kernel[r][c] = 1.0f;
        divisor = 9.0f;
        break;
    case RadiantFilterType::Smooth2:
        kernel[0][0] = 1; kernel[0][1] = 2; kernel[0][2] = 1;
        kernel[1][0] = 2; kernel[1][1] = 4; kernel[1][2] = 2;
        kernel[2][0] = 1; kernel[2][1] = 2; kernel[2][2] = 1;
        divisor = 16.0f;
        break;
    case RadiantFilterType::Smooth3:
        kernel[0][0] = 1; kernel[0][1] = 1; kernel[0][2] = 1;
        kernel[1][0] = 1; kernel[1][1] = 2; kernel[1][2] = 1;
        kernel[2][0] = 1; kernel[2][1] = 1; kernel[2][2] = 1;
        divisor = 10.0f;
        break;
    case RadiantFilterType::Edge1:
        kernel[0][0] =  0; kernel[0][1] =  1; kernel[0][2] =  0;
        kernel[1][0] =  1; kernel[1][1] = -4; kernel[1][2] =  1;
        kernel[2][0] =  0; kernel[2][1] =  1; kernel[2][2] =  0;
        break;
    case RadiantFilterType::Edge2:
        kernel[0][0] = -1; kernel[0][1] = -1; kernel[0][2] = -1;
        kernel[1][0] = -1; kernel[1][1] =  8; kernel[1][2] = -1;
        kernel[2][0] = -1; kernel[2][1] = -1; kernel[2][2] = -1;
        break;
    case RadiantFilterType::Edge3:
        kernel[0][0] = -1; kernel[0][1] =  0; kernel[0][2] =  1;
        kernel[1][0] = -2; kernel[1][1] =  0; kernel[1][2] =  2;
        kernel[2][0] = -1; kernel[2][1] =  0; kernel[2][2] =  1;
        break;
    case RadiantFilterType::EmbossN:
        kernel[0][0] =  0; kernel[0][1] =  1; kernel[0][2] =  0;
        kernel[1][0] =  0; kernel[1][1] =  0; kernel[1][2] =  0;
        kernel[2][0] =  0; kernel[2][1] = -1; kernel[2][2] =  0;
        bias = 128.0f;
        break;
    case RadiantFilterType::EmbossW:
        kernel[0][0] =  0; kernel[0][1] =  0; kernel[0][2] =  0;
        kernel[1][0] =  1; kernel[1][1] =  0; kernel[1][2] = -1;
        kernel[2][0] =  0; kernel[2][1] =  0; kernel[2][2] =  0;
        bias = 128.0f;
        break;
    case RadiantFilterType::EmbossD:
        kernel[0][0] =  1; kernel[0][1] =  0; kernel[0][2] =  0;
        kernel[1][0] =  0; kernel[1][1] =  0; kernel[1][2] =  0;
        kernel[2][0] =  0; kernel[2][1] =  0; kernel[2][2] = -1;
        bias = 128.0f;
        break;
    case RadiantFilterType::Emboss1:
        kernel[0][0] = -2; kernel[0][1] = -1; kernel[0][2] =  0;
        kernel[1][0] = -1; kernel[1][1] =  1; kernel[1][2] =  1;
        kernel[2][0] =  0; kernel[2][1] =  1; kernel[2][2] =  2;
        bias = 128.0f;
        break;
    case RadiantFilterType::Emboss2:
        kernel[0][0] =  2; kernel[0][1] =  1; kernel[0][2] =  0;
        kernel[1][0] =  1; kernel[1][1] =  1; kernel[1][2] = -1;
        kernel[2][0] =  0; kernel[2][1] = -1; kernel[2][2] = -2;
        bias = 128.0f;
        break;
    default:
        return src;
    }

    QImage formatted = src.convertToFormat(QImage::Format_RGB32);
    QImage dst = formatted;
    const int w = formatted.width();
    const int h = formatted.height();

    for (int y = 1; y < h - 1; ++y)
    {
        const QRgb * prev = reinterpret_cast<const QRgb*>(formatted.scanLine(y - 1));
        const QRgb * curr = reinterpret_cast<const QRgb*>(formatted.scanLine(y));
        const QRgb * next = reinterpret_cast<const QRgb*>(formatted.scanLine(y + 1));
        QRgb * out = reinterpret_cast<QRgb*>(dst.scanLine(y));

        for (int x = 1; x < w - 1; ++x)
        {
            float r = 0, g = 0, b = 0;

            const QRgb p[3][3] = {
                { prev[x - 1], prev[x], prev[x + 1] },
                { curr[x - 1], curr[x], curr[x + 1] },
                { next[x - 1], next[x], next[x + 1] }
            };

            for (int ky = 0; ky < 3; ++ky)
            {
                for (int kx = 0; kx < 3; ++kx)
                {
                    r += qRed(p[ky][kx])   * kernel[ky][kx];
                    g += qGreen(p[ky][kx]) * kernel[ky][kx];
                    b += qBlue(p[ky][kx])  * kernel[ky][kx];
                }
            }

            int ir = static_cast<int>(r / divisor + bias);
            int ig = static_cast<int>(g / divisor + bias);
            int ib = static_cast<int>(b / divisor + bias);

            ir = std::max(0, std::min(255, ir));
            ig = std::max(0, std::min(255, ig));
            ib = std::max(0, std::min(255, ib));

            out[x] = qRgb(ir, ig, ib);
        }
    }

    return dst;
}
