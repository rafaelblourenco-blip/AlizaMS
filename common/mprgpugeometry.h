#ifndef ALIZA_MPR_GPU_GEOMETRY_H
#define ALIZA_MPR_GPU_GEOMETRY_H

#include "mprslabitk.h"

// Coordinates passed to the GPU are relative to the input region, not absolute
// ITK indices. Transform in double precision before converting to shader floats.
namespace mpr_gpu {
struct SamplingGeometry {
    mpr_geometry::Vector3 first, du, dv, normal_per_mm;
    unsigned int width = 0, height = 0;
    double thickness = 0, sample_step = 0;
    int half_samples = 0;
};

template<class Image>
bool sampling_geometry(const Image *image, mpr_geometry::Vector3 center,
                       mpr_geometry::Vector3 u, mpr_geometry::Vector3 v,
                       unsigned int width, unsigned int height,
                       double spacing_u, double spacing_v, double thickness,
                       SamplingGeometry &out)
{
    using namespace mpr_geometry;
    if (!image || !width || !height || !std::isfinite(spacing_u) || spacing_u <= 0 ||
        !std::isfinite(spacing_v) || spacing_v <= 0 || !std::isfinite(thickness) || thickness < 0 ||
        !std::isfinite(dot(u,u)) || !std::isfinite(dot(v,v)) || !std::isfinite(dot(u,v)) ||
        std::abs(dot(u,u)-1) > 1e-6 || std::abs(dot(v,v)-1) > 1e-6 || std::abs(dot(u,v)) > 1e-6)
        return false;
    const auto spacing=image->GetSpacing();
    for (unsigned i=0; i<3; ++i) if (!(spacing[i]>0) || !std::isfinite(spacing[i])) return false;
    const double step=(std::min)({spacing[0],spacing[1],spacing[2]});
    const double n=std::ceil(thickness*0.5/step);
    if (!std::isfinite(n) || n>65536) return false;
    const auto region=image->GetLargestPossibleRegion();
    const auto start=region.GetIndex();
    const auto size=region.GetSize();
    for (unsigned i=0; i<3; ++i) if (!size[i]) return false;
    auto index = [&](Vector3 p, Vector3 &result) {
        typename Image::PointType point;
        point[0]=p.x; point[1]=p.y; point[2]=p.z;
        itk::ContinuousIndex<double,3> value;
        image->TransformPhysicalPointToContinuousIndex(point,value);
        result={value[0]-start[0],value[1]-start[1],value[2]-start[2]};
        return std::isfinite(result.x) && std::isfinite(result.y) && std::isfinite(result.z);
    };
    const Vector3 first=pixel_position(center,u,v,0,0,width,height,spacing_u,spacing_v);
    Vector3 base, alongU, alongV, alongN;
    if (!index(first,base) || !index(add(first,multiply(u,spacing_u)),alongU) ||
        !index(add(first,multiply(v,spacing_v)),alongV) || !index(add(first,cross(u,v)),alongN)) return false;
    auto delta=[&](Vector3 a) { return add(a,multiply(base,-1)); };
    SamplingGeometry candidate;
    candidate.first=base; candidate.du=delta(alongU); candidate.dv=delta(alongV);
    candidate.normal_per_mm=delta(alongN); candidate.width=width; candidate.height=height;
    candidate.thickness=thickness; candidate.half_samples=static_cast<int>(n);
    candidate.sample_step=n ? thickness*0.5/n : 0;
    out=candidate;
    return true;
}
}
#endif
