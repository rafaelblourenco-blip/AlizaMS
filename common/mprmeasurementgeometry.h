#ifndef ALIZA_MPR_MEASUREMENT_GEOMETRY_H
#define ALIZA_MPR_MEASUREMENT_GEOMETRY_H
#include <QPainterPath>
#include <QPolygonF>
#include <QLineF>
#include <QVector>
#include <cmath>
#include <algorithm>

namespace mpr_measurement {
inline bool valid_spacing(double u, double v) { return std::isfinite(u) && std::isfinite(v) && u > 0 && v > 0; }
inline double angle_degrees(QPointF a, QPointF vertex, QPointF b, double u, double v) {
    const QPointF x((a.x()-vertex.x())*u,(a.y()-vertex.y())*v), y((b.x()-vertex.x())*u,(b.y()-vertex.y())*v);
    if (!valid_spacing(u,v) || std::hypot(x.x(),x.y()) == 0 || std::hypot(y.x(),y.y()) == 0) return -1;
    return std::atan2(std::abs(x.x()*y.y()-x.y()*y.x()),x.x()*y.x()+x.y()*y.y()) * 180.0 / std::acos(-1.0);
}
inline double ellipse_area(const QRectF &r, double u, double v) {
    if (!valid_spacing(u,v) || r.width() <= 0 || r.height() <= 0) return -1;
    return std::acos(-1.0)*r.width()*u*r.height()*v/4.0;
}
inline QPainterPath rounded_polygon(const QVector<QPointF> &points) {
    QPainterPath path;
    if (points.size()<3) return path;
    path.moveTo((points.last()+points.first())/2.0);
    for (int i=0;i<points.size();++i) path.quadTo(points[i],(points[i]+points[(i+1)%points.size()])/2.0);
    path.closeSubpath(); return path;
}
inline bool simple_area(const QPainterPath &path, double u, double v, double &area) {
    if (!valid_spacing(u,v)) return false;
    const auto polygons=path.toSubpathPolygons();
    if (polygons.size()!=1) return false;
    QPolygonF p=polygons.first();
    if (p.size() < 4) return false;
    if (p.first()!=p.last()) p.append(p.first());
    const int count=p.size()-1;
    double sum=0;
    for (int i=0;i<count;++i) {
        if (QLineF(p[i],p[i+1]).length() < 1e-9) return false;
        sum += p[i].x()*p[i+1].y()-p[i+1].x()*p[i].y();
        for (int j=i+1;j<count;++j) {
            if (j==i+1 || (i==0 && j==count-1)) continue;
            QPointF hit;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            if (QLineF(p[i],p[i+1]).intersects(QLineF(p[j],p[j+1]), &hit)==QLineF::BoundedIntersection) return false;
#else
            if (QLineF(p[i],p[i+1]).intersect(QLineF(p[j],p[j+1]), &hit)==QLineF::BoundedIntersection) return false;
#endif
        }
    }
    area=std::abs(sum)*u*v/2.0;
    return std::isfinite(area) && area>0;
}
template<class Valid> bool valid_segments(const QVector<QPointF> &points, Valid valid) {
    for (int i=1;i<points.size();++i) {
        const QPointF d=points[i]-points[i-1];
        const int n=std::max(1,static_cast<int>(std::ceil(std::max(std::abs(d.x()),std::abs(d.y()))*2)));
        for (int j=0;j<=n;++j) if (!valid(points[i-1]+d*(double(j)/n))) return false;
    }
    return true;
}
}
#endif
