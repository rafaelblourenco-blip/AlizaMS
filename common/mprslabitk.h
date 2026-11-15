#ifndef ALIZA_MPR_SLAB_ITK_H
#define ALIZA_MPR_SLAB_ITK_H

#include "mprslab.h"
#include <itkImage.h>
#include <itkLinearInterpolateImageFunction.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mpr_slab {
// The mask distinguishes pixels outside the input from valid zero intensity.
struct ProjectionImage {
    itk::Image<double, 2>::Pointer pixels;
    itk::Image<unsigned char, 2>::Pointer valid;
    bool all_valid = false;
};
// Scalar output remains double through projection; W/L is applied afterwards.
// An empty result means invalid geometry or an excessive work request.
template <typename TInput>
ProjectionImage render_itk(
    const TInput *input, Vector3 center, Vector3 u, Vector3 v,
    unsigned int width, unsigned int height, double spacing_u, double spacing_v,
    double thickness_mm, Mode mode)
{
    using Output = itk::Image<double, 2>;
    using Mask = itk::Image<unsigned char, 2>;
    using Interpolator = itk::LinearInterpolateImageFunction<TInput, double>;
    if (!input || !width || !height || !std::isfinite(spacing_u) || spacing_u <= 0 ||
        !std::isfinite(spacing_v) || spacing_v <= 0 || !std::isfinite(thickness_mm) || thickness_mm < 0)
        return {};
    const auto finite_vector = [](Vector3 a) {
        return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z);
    };
    if (!finite_vector(center) || !finite_vector(u) || !finite_vector(v) ||
        std::abs(mpr_geometry::dot(u,u)-1.0) > 1e-6 ||
        std::abs(mpr_geometry::dot(v,v)-1.0) > 1e-6 ||
        std::abs(mpr_geometry::dot(u,v)) > 1e-6) return {};
    const auto spacing = input->GetSpacing();
    for (unsigned int axis=0; axis<3; ++axis)
        if (!(spacing[axis] > 0) || !std::isfinite(spacing[axis])) return {};
    const double step = std::min({spacing[0], spacing[1], spacing[2]});
    const double half_count = std::ceil((thickness_mm * 0.5) / step);
    const double work = static_cast<double>(width) * height * (2 * half_count + 1);
    const double pixels = static_cast<double>(width) * height;
    if (half_count > 65536.0 || !std::isfinite(work) || work > 30000000.0 ||
        pixels > 4194304.0 ||
        (mode != Mode::Maximum && mode != Mode::Minimum && mode != Mode::Mean)) return {};
    const Vector3 normal = mpr_geometry::cross(u, v);
    auto interpolator = Interpolator::New();
    interpolator->SetInputImage(input);
    const auto sampler = [&](Vector3 p, double & value) {
        typename TInput::PointType point;
        point[0] = p.x; point[1] = p.y; point[2] = p.z;
        if (!interpolator->IsInsideBuffer(point)) return false;
        value = static_cast<double>(interpolator->Evaluate(point));
        return true;
    };
    auto output = Output::New();
    Output::IndexType start; start.Fill(0);
    Output::SizeType size; size[0] = width; size[1] = height;
    Output::RegionType region; region.SetIndex(start); region.SetSize(size);
    output->SetRegions(region);
    Output::SpacingType outSpacing; outSpacing[0] = spacing_u; outSpacing[1] = spacing_v;
    output->SetSpacing(outSpacing);
    output->Allocate();
    auto mask = Mask::New();
    mask->SetRegions(region);
    mask->SetSpacing(outSpacing);
    mask->Allocate();
    bool all_valid = true;
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            const Vector3 point = mpr_geometry::pixel_position(center, u, v, x, y,
                                                                width, height, spacing_u, spacing_v);
            const Result result = project(point, normal, thickness_mm, step, mode, sampler);
            Output::IndexType index; index[0] = x; index[1] = y;
            output->SetPixel(index, result.valid ? result.value : 0.0);
            mask->SetPixel(index, result.valid ? 1 : 0);
            all_valid = all_valid && result.valid;
        }
    }
    ProjectionImage result;
    result.pixels = output;
    result.valid = mask;
    result.all_valid = all_valid;
    return result;
}
}
#endif
