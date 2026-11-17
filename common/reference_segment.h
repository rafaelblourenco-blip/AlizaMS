#ifndef ALIZA_REFERENCE_SEGMENT_H
#define ALIZA_REFERENCE_SEGMENT_H
#include "mprgeometry.h"
#include <cmath>
#include <limits>
namespace mpr_geometry {
// Rectangle bounds are physical millimetres along the plane's unit basis.
struct PlaneRectangle { Plane plane; double u_min, u_max, v_min, v_max; };
struct ReferenceSegment { Vector3 start, end; };
inline bool clip_reference_interval(double p, double d, double lo, double hi, double &t0, double &t1) {
    if (!std::isfinite(p) || !std::isfinite(d) || !std::isfinite(lo) || !std::isfinite(hi) || lo > hi) return false;
    if (std::abs(d) < 1e-12) return p >= lo && p <= hi;
    double a=(lo-p)/d, b=(hi-p)/d;
    if (a>b) { const double t=a; a=b; b=t; }
    if (a>t0) t0=a;
    if (b<t1) t1=b;
    return t0<=t1;
}
// DICOM corner order is top-left, top-right, bottom-right, bottom-left.
inline bool rectangle_from_corners(const Vector3 (&corners)[4], PlaneRectangle &result) {
    const Vector3 du=add(corners[1],multiply(corners[0],-1));
    const Vector3 dv=add(corners[3],multiply(corners[0],-1));
    const double lu=std::sqrt(dot(du,du)), lv=std::sqrt(dot(dv,dv));
    if (!std::isfinite(lu) || !std::isfinite(lv) || lu < 1e-9 || lv < 1e-9) return false;
    const Vector3 u=multiply(du,1.0/lu), v=multiply(dv,1.0/lv);
    if (std::abs(dot(u,v))>1e-6) return false;
    const Vector3 expected=add(corners[0],add(du,dv));
    const Vector3 residual=add(corners[2],multiply(expected,-1));
    if (!std::isfinite(dot(residual,residual)) || dot(residual,residual)>1e-6) return false;
    result={{corners[0],u,v},0,lu,0,lv};
    return true;
}
inline bool bounded_reference_segment(const PlaneRectangle &view, const PlaneRectangle &slice, ReferenceSegment &result) {
    const auto valid_plane = [](const Plane &p) {
        const auto finite = [](Vector3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); };
        return finite(p.center) && finite(p.u) && finite(p.v) &&
               std::abs(dot(p.u,p.u)-1.0)<1e-6 && std::abs(dot(p.v,p.v)-1.0)<1e-6 &&
               std::abs(dot(p.u,p.v))<1e-6;
    };
    if (!valid_plane(view.plane) || !valid_plane(slice.plane)) return false;
    Line2D line;
    if (!reference_line(view.plane,slice.plane,line)) return false;
    const Vector3 p=add(view.plane.center, add(multiply(view.plane.u,line.u),multiply(view.plane.v,line.v)));
    const Vector3 d=add(multiply(view.plane.u,line.du),multiply(view.plane.v,line.dv));
    const Vector3 delta=add(p,multiply(slice.plane.center,-1));
    double a=-(std::numeric_limits<double>::infinity)(), b=(std::numeric_limits<double>::infinity)();
    if (!clip_reference_interval(line.u,line.du,view.u_min,view.u_max,a,b) ||
        !clip_reference_interval(line.v,line.dv,view.v_min,view.v_max,a,b) ||
        !clip_reference_interval(dot(delta,slice.plane.u),dot(d,slice.plane.u),slice.u_min,slice.u_max,a,b) ||
        !clip_reference_interval(dot(delta,slice.plane.v),dot(d,slice.plane.v),slice.v_min,slice.v_max,a,b) ||
        !std::isfinite(a) || !std::isfinite(b) || b-a < 1e-9) return false;
    result.start=add(p,multiply(d,a)); result.end=add(p,multiply(d,b));
    return true;
}
}
#endif
