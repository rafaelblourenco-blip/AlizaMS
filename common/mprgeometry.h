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
}
#endif
