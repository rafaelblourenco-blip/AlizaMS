#ifndef ALIZA_MPR_GEOMETRY_H
#define ALIZA_MPR_GEOMETRY_H

#include <cmath>

// All vectors and positions here use physical units, not display pixels.
namespace mpr_geometry {
struct Vector3 { double x, y, z; };
inline Vector3 add(Vector3 a, Vector3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vector3 multiply(Vector3 a, double k) { return {a.x*k, a.y*k, a.z*k}; }
inline double dot(Vector3 a, Vector3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vector3 cross(Vector3 a, Vector3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
inline Vector3 rotate(Vector3 value, Vector3 unit_axis, double angle) {
    const double c=std::cos(angle), s=std::sin(angle);
    return add(add(multiply(value,c), multiply(cross(unit_axis,value),s)),
               multiply(unit_axis,dot(unit_axis,value)*(1.0-c)));
}
// Convert scene deltas to physical coordinates before computing the angle.
// The caller must first map screen positions through the inverse viewport transform.
inline double scene_angle(double dx, double dy, double spacing_u, double spacing_v) {
    return std::atan2(dy*spacing_v, dx*spacing_u);
}
// Offset the output centre so the pivot remains at the same output pixel after rotation.
inline Vector3 output_center(Vector3 pivot, Vector3 u, Vector3 v,
                             double pivot_u, double pivot_v,
                             unsigned width, unsigned height,
                             double spacing_u, double spacing_v) {
    return add(pivot, add(multiply(u,((width-1)*0.5-pivot_u)*spacing_u),
                          multiply(v,((height-1)*0.5-pivot_v)*spacing_v)));
}
inline Vector3 pixel_position(Vector3 center, Vector3 u, Vector3 v,
                              double pixel_u, double pixel_v,
                              unsigned width, unsigned height,
                              double spacing_u, double spacing_v) {
    return add(center, add(multiply(u,(pixel_u-(width-1)*0.5)*spacing_u),
                           multiply(v,(pixel_v-(height-1)*0.5)*spacing_v)));
}
// Intersect a physical target plane with a displayed plane. Coordinates in
// Line2D are millimetres along the displayed plane's unit U and V vectors.
struct Plane { Vector3 center, u, v; };
struct Line2D { double u, v, du, dv; };
// All three planes share the same physical pivot; each plane retains its own
// orientation. Pixel spacing and the viewport transform are separate state.
struct PlaneSet {
    Vector3 pivot;
    Plane planes[3];
    void set_pivot(Vector3 point) {
        pivot = point;
        for (auto &plane : planes) plane.center = point;
    }
    bool rotate_target(unsigned source, unsigned target, double angle) {
        if (source >= 3 || target >= 3 || source == target || !std::isfinite(angle)) return false;
        const Vector3 axis = cross(planes[source].u, planes[source].v);
        if (std::abs(dot(axis, axis) - 1.0) > 1e-6) return false;
        planes[target].u = rotate(planes[target].u, axis, angle);
        planes[target].v = rotate(planes[target].v, axis, angle);
        return true;
    }
};
inline bool reference_line(const Plane &view, const Plane &target, Line2D &line) {
    const Vector3 normal = cross(target.u, target.v);
    const double a = dot(normal, view.u);
    const double b = dot(normal, view.v);
    const double squared = a*a + b*b;
    if (!std::isfinite(squared) || squared < 1e-20) return false;
    const double c = dot(normal, add(view.center, multiply(target.center, -1.0)));
    if (!std::isfinite(c)) return false;
    line.u = -c*a/squared;
    line.v = -c*b/squared;
    const double scale = std::sqrt(squared);
    line.du = -b/scale;
    line.dv = a/scale;
    return true;
}

}
#endif
