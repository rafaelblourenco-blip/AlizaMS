#ifndef ALIZA_MPR_SLAB_ITK_H
#define ALIZA_MPR_SLAB_ITK_H

#include "mprslab.h"
#include <itkImage.h>
#include <itkLinearInterpolateImageFunction.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <thread>
#include <vector>

namespace mpr_slab {
// Set the shared physical pivot only when it lies within voxel-center bounds.
template <typename TInput>
bool set_pivot_itk(mpr_geometry::PlaneSet &state, mpr_geometry::Vector3 candidate,
                   const TInput *input, double *relative_voxel = nullptr)
{
    if (!input || !std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z)) return false;
    typename TInput::PointType point; point[0]=candidate.x; point[1]=candidate.y; point[2]=candidate.z;
    itk::ContinuousIndex<double,3> index;
    input->TransformPhysicalPointToContinuousIndex(point,index);
    const auto region=input->GetLargestPossibleRegion();
    const auto start=region.GetIndex(); const auto size=region.GetSize();
    for (unsigned int i=0; i<3; ++i) {
        if (!size[i] || !std::isfinite(index[i]) || index[i]<static_cast<double>(start[i])-1e-8 ||
            index[i]>static_cast<double>(start[i])+static_cast<double>(size[i]-1)+1e-8) return false;
    }
    state.set_pivot(candidate);
    if (relative_voxel) for (unsigned int i=0; i<3; ++i) relative_voxel[i]=index[i]-start[i];
    return true;
}
// Move a shared pivot in millimetres along a plane normal, preserving bounds.
template <typename TInput>
bool advance_pivot_itk(mpr_geometry::PlaneSet &state, unsigned int plane,
                       double distance_mm, const TInput *input)
{
    if (!input || plane >= 3 || !std::isfinite(distance_mm)) return false;
    auto normal=mpr_geometry::cross(state.planes[plane].u,state.planes[plane].v);
    const double length=std::sqrt(mpr_geometry::dot(normal,normal));
    if (!std::isfinite(length) || length < 1e-12) return false;
    normal=mpr_geometry::multiply(normal,1.0/length);
    return set_pivot_itk(state,mpr_geometry::add(state.pivot,mpr_geometry::multiply(normal,distance_mm)),input);
}

// Slider bounds are expressed in whole steps from its current value. One step
// has unit length in continuous voxel-index space along the physical normal.
template <typename TInput>
bool navigation_bounds_itk(const mpr_geometry::PlaneSet &state, unsigned plane,
                           int sign, int current, const TInput *input,
                           int &minimum, int &maximum)
{
    if (!input || plane>=3 || (sign!=1 && sign!=-1)) return false;
    auto normal=mpr_geometry::cross(state.planes[plane].u,state.planes[plane].v);
    const double length=std::sqrt(mpr_geometry::dot(normal,normal));
    if (!(length>1e-12) || !std::isfinite(length)) return false;
    normal=mpr_geometry::multiply(normal,sign/length);
    typename TInput::PointType p,q;
    p[0]=state.pivot.x; p[1]=state.pivot.y; p[2]=state.pivot.z;
    q[0]=p[0]+normal.x; q[1]=p[1]+normal.y; q[2]=p[2]+normal.z;
    itk::ContinuousIndex<double,3> i0,i1;
    input->TransformPhysicalPointToContinuousIndex(p,i0);
    input->TransformPhysicalPointToContinuousIndex(q,i1);
    const auto region=input->GetLargestPossibleRegion();
    const auto start=region.GetIndex(); const auto size=region.GetSize();
    double low=-(std::numeric_limits<double>::infinity)();
    double high=(std::numeric_limits<double>::infinity)();
    double squared=0;
    for (unsigned a=0; a<3; ++a) {
        if (!size[a] || !std::isfinite(i0[a]) || !std::isfinite(i1[a])) return false;
        const double d=i1[a]-i0[a], lo=static_cast<double>(start[a]);
        const double hi=lo+static_cast<double>(size[a]-1);
        if (i0[a]<lo-1e-8 || i0[a]>hi+1e-8) return false;
        squared+=d*d;
        if (std::abs(d)<1e-15) continue;
        double t0=(lo-i0[a])/d, t1=(hi-i0[a])/d;
        if (t0>t1) { const double tmp=t0; t0=t1; t1=tmp; }
        if (t0>low) low=t0;
        if (t1<high) high=t1;
    }
    if (!(squared>0) || !std::isfinite(low) || !std::isfinite(high)) return false;
    const double step=1.0/std::sqrt(squared);
    const double minValue=static_cast<double>(current)+std::ceil(low/step-1e-9);
    const double maxValue=static_cast<double>(current)+std::floor(high/step+1e-9);
    if (minValue < (std::numeric_limits<int>::min)() || maxValue > (std::numeric_limits<int>::max)() || minValue>maxValue) return false;
    minimum=static_cast<int>(minValue); maximum=static_cast<int>(maxValue);
    return true;
}

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
    double thickness_mm, Mode mode, const std::atomic<bool> *cancel = nullptr)
{
    using Output = itk::Image<double, 2>;
    using Mask = itk::Image<unsigned char, 2>;
    using Interpolator = itk::LinearInterpolateImageFunction<TInput, double>;
    if ((cancel && cancel->load()) || !input || !width || !height || !std::isfinite(spacing_u) || spacing_u <= 0 ||
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
    const double step = (std::min)({spacing[0], spacing[1], spacing[2]});
    const double half_count = std::ceil((thickness_mm * 0.5) / step);
    const double work = static_cast<double>(width) * height * (2 * half_count + 1);
    const double pixels = static_cast<double>(width) * height;
    if (half_count > 65536.0 || !std::isfinite(work) || work > 30000000.0 ||
        pixels > 4194304.0 ||
        (mode != Mode::Maximum && mode != Mode::Minimum && mode != Mode::Mean)) return {};
    const Vector3 normal = mpr_geometry::cross(u, v);
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
    std::atomic<bool> all_valid{true};
    // Each worker owns its interpolator and writes a disjoint set of rows.
    // Bound parallelism because three MPR views can render concurrently.
    const unsigned int available = (std::max)(1u, std::thread::hardware_concurrency());
    const unsigned int count = (std::min)({height, 4u, available});
    auto render_rows = [&](unsigned int first, unsigned int last) {
        auto interpolator = Interpolator::New();
        interpolator->SetInputImage(input);
        const auto sampler = [&](Vector3 p, double & value) {
            typename TInput::PointType point;
            point[0] = p.x; point[1] = p.y; point[2] = p.z;
            if (!interpolator->IsInsideBuffer(point)) return false;
            value = static_cast<double>(interpolator->Evaluate(point));
            return true;
        };
        for (unsigned int y = first; y < last; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                if (cancel && cancel->load()) return;
                const Vector3 point = mpr_geometry::pixel_position(center, u, v, x, y,
                                                                    width, height, spacing_u, spacing_v);
                const Result sample = project(point, normal, thickness_mm, step, mode, sampler);
                const std::size_t offset = static_cast<std::size_t>(y) * width + x;
                output->GetBufferPointer()[offset] = sample.valid ? sample.value : 0.0;
                mask->GetBufferPointer()[offset] = sample.valid ? 1 : 0;
                if (!sample.valid) all_valid.store(false, std::memory_order_relaxed);
            }
        }
    };
    std::vector<std::thread> threads;
    for (unsigned int i = 1; i < count; ++i)
        threads.emplace_back(render_rows, height * i / count, height * (i + 1) / count);
    render_rows(0, height / count);
    for (auto &thread : threads) thread.join();
    if (cancel && cancel->load()) return {};
    ProjectionImage result;
    result.pixels = output;
    result.valid = mask;
    result.all_valid = all_valid.load();
    return result;
}
}
#endif
