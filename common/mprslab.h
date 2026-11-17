#ifndef ALIZA_MPR_SLAB_H
#define ALIZA_MPR_SLAB_H

#include "mprgeometry.h"
#include <cmath>
#include <cstddef>

// Scalar projection in patient-space millimetres. The sampler must transform
// the supplied physical point to the volume and reject out-of-volume points.
namespace mpr_slab {
using mpr_geometry::Vector3;
enum class Mode { Maximum, Minimum, Mean };
struct Result {
    bool valid = false;
    double value = 0.0;
    std::size_t samples = 0;
};

// Samples both slab boundaries and the central plane. Actual spacing is no
// larger than max_step_mm and the positions are symmetric about the centre.
// Equal sample weights define the discrete mean; invalid samples do not enter
// any aggregate. A zero-thickness slab samples the centre exactly once.
// No assumption is made about the input intensity range or pixel type.
template<class Sampler>
Result project(Vector3 center, Vector3 normal, double thickness_mm,
               double max_step_mm, Mode mode, const Sampler & sample)
{
    Result out;
    if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(center.z) ||
        !std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z) ||
        !std::isfinite(thickness_mm) || thickness_mm < 0.0 ||
        !std::isfinite(max_step_mm) || max_step_mm <= 0.0 ||
        (mode != Mode::Maximum && mode != Mode::Minimum && mode != Mode::Mean))
        return out;
    const double length = std::hypot(std::hypot(normal.x, normal.y), normal.z);
    if (!(length > 0.0) || !std::isfinite(length)) return out;
    // Divide each component directly: the reciprocal of a subnormal length
    // may overflow even though the normalized components are representable.
    normal = {normal.x / length, normal.y / length, normal.z / length};
    const double half = thickness_mm * 0.5;
    const double half_count = std::ceil(half / max_step_mm);
    // Reject unreasonable requests instead of blocking the UI indefinitely.
    if (!std::isfinite(half_count) || half_count > 65536.0) return out;
    const int n = static_cast<int>(half_count);
    const double step = n ? half / n : 0.0;
    for (int i = -n; i <= n; ++i) {
        const Vector3 point = mpr_geometry::add(center, mpr_geometry::multiply(normal, i * step));
        // Never pass non-finite coordinates to an interpolator. Reject the
        // whole projection, rather than silently aggregating a partial slab.
        if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z))
            return Result{};
        double value = 0.0;
        if (!sample(point, value) || !std::isfinite(value)) continue;
        ++out.samples;
        if (out.samples == 1) out.value = value;
        else if (mode == Mode::Maximum && value > out.value) out.value = value;
        else if (mode == Mode::Minimum && value < out.value) out.value = value;
        else if (mode == Mode::Mean) out.value = out.value * (1.0 - 1.0 / static_cast<double>(out.samples)) +
                                      value / static_cast<double>(out.samples);
    }
    out.valid = out.samples != 0;
    return out;
}
}
#endif
