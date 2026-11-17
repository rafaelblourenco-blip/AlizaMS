#ifndef ALIZA_SCULPT_MASK_H
#define ALIZA_SCULPT_MASK_H

#include <QMatrix4x4>
#include <QPolygonF>
#include <QVector4D>
#include <vector>
#include <limits>
#include <cmath>
#include <exception>

// Private visualization mask: no source voxels or shared texture are changed.
// textureToClip maps normalized texture coordinates to the clip coordinates of
// the camera at the time the outline was drawn. The mask persists after rotation.
class SculptMask
{
public:
    bool apply(int width, int height, int depth, const QMatrix4x4 &textureToClip,
               const QPolygonF &outline, bool deleteInside)
    {
        if (width < 1 || height < 1 || depth < 1 || outline.size() < 3) return false;
        double area = 0.0;
        for (int i = 0; i < outline.size(); ++i)
        {
            const QPointF a = outline.at(i), b = outline.at((i + 1) % outline.size());
            if (!std::isfinite(a.x()) || !std::isfinite(a.y())) return false;
            area += a.x() * b.y() - b.x() * a.y();
        }
        if (std::abs(area) < 1e-8) return false;
        const size_t w = static_cast<size_t>(width), h = static_cast<size_t>(height), d = static_cast<size_t>(depth);
        if (w > std::numeric_limits<size_t>::max() / h || w * h > std::numeric_limits<size_t>::max() / d) return false;
        const size_t count = w * h * d;
        if (!mask_.empty() && (width != width_ || height != height_ || depth != depth_)) return false;
        std::vector<unsigned char> next;
        try { next = mask_.empty() ? std::vector<unsigned char>(count, 255) : mask_; }
        catch (const std::exception &) { return false; }
        const QRectF bounds = outline.boundingRect();
        for (int z = 0; z < depth; ++z)
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    const QVector4D p = textureToClip * QVector4D((x + 0.5f) / width, (y + 0.5f) / height, (z + 0.5f) / depth, 1.0f);
                    if (p.w() <= 0.0f || !std::isfinite(p.w())) continue;
                    const double px = p.x() / p.w(), py = p.y() / p.w();
                    if (!std::isfinite(px) || !std::isfinite(py)) continue;
                    const QPointF projected(px, py);
                    const bool inside = bounds.contains(projected) && outline.containsPoint(projected, Qt::OddEvenFill);
                    if (inside == deleteInside)
                        next[(static_cast<size_t>(z) * h + static_cast<size_t>(y)) * w + static_cast<size_t>(x)] = 0;
                }
        mask_.swap(next);
        width_ = width; height_ = height; depth_ = depth;
        return true;
    }
    void clear() { mask_.clear(); width_ = height_ = depth_ = 0; }
    bool empty() const { return mask_.empty(); }
    const std::vector<unsigned char> &data() const { return mask_; }
private:
    int width_ = 0, height_ = 0, depth_ = 0;
    std::vector<unsigned char> mask_;
};
#endif
