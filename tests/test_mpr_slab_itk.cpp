#include "../common/mprslabitk.h"
#include <itkImageRegionIterator.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
using Image = itk::Image<short, 3>;
using namespace mpr_slab;
static void require(bool condition, const char *message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
static bool approx_equal(double a, double b) { return std::abs(a-b) < 1e-7; }
int main() {
    auto image = Image::New();
    Image::IndexType start; start[0]=4; start[1]=7; start[2]=11;
    Image::SizeType size; size.Fill(3);
    Image::RegionType region; region.SetIndex(start); region.SetSize(size);
    image->SetRegions(region);
    Image::SpacingType spacing; spacing[0]=2; spacing[1]=3; spacing[2]=5;
    image->SetSpacing(spacing);
    Image::PointType origin; origin[0]=30; origin[1]=-60; origin[2]=8;
    image->SetOrigin(origin);
    Image::DirectionType direction; direction.SetIdentity();
    direction[0][0]=0; direction[1][0]=1; direction[0][1]=-1; direction[1][1]=0;
    image->SetDirection(direction); image->Allocate();
    itk::ImageRegionIterator<Image> it(image,region);
    for (it.GoToBegin(); !it.IsAtEnd(); ++it)
        it.Set(static_cast<short>(-1000+100*(it.GetIndex()[2]-start[2])));
    Image::IndexType middle; middle[0]=5; middle[1]=8; middle[2]=12;
    Image::PointType p; image->TransformIndexToPhysicalPoint(middle,p);
    const Vector3 center{p[0],p[1],p[2]}, u{0,1,0}, v{-1,0,0};
    auto value = [&](Mode mode, double thickness) {
        auto out=render_itk<Image>(image.GetPointer(),center,u,v,1,1,2,3,thickness,mode);
        require(out.pixels.IsNotNull() && out.valid.IsNotNull() && out.all_valid,"projection allocation");
        itk::Index<2> i; i.Fill(0); return out.pixels->GetPixel(i);
    };
    require(approx_equal(value(Mode::Maximum,10),-800),"ITK MIP");
    require(approx_equal(value(Mode::Minimum,10),-1000),"ITK MinIP");
    require(approx_equal(value(Mode::Mean,10),-900),"ITK mean");
    require(approx_equal(value(Mode::Mean,0),-900),"ITK thin");
    const double q=std::sqrt(0.5);
    // Cross(u,v)=(q,0,q), testing oblique interpolation in physical space.
    const Vector3 ou{0,1,0}, ov{-q,0,q};
    auto oblique=render_itk<Image>(image.GetPointer(),center,ou,ov,1,1,2,3,4,Mode::Maximum);
    require(oblique.pixels.IsNotNull() && oblique.all_valid,"ITK oblique allocation");
    itk::Index<2> i; i.Fill(0);
    require(approx_equal(oblique.pixels->GetPixel(i),-900+40*q),"ITK oblique interpolation");
    // Three-view phantom with shared physical pivot and a rotated coronal plane.
    mpr_geometry::PlaneSet planes;
    planes.planes[0]={center,{-1,0,0},{0,0,1}};
    planes.planes[1]={center,{0,1,0},{0,0,1}};
    planes.planes[2]={center,{0,1,0},{-1,0,0}};
    planes.set_pivot(center);
    require(planes.rotate_target(2,1,std::acos(-1.0)/6),"rotate phantom plane");
    for (unsigned view=0; view<3; ++view) {
        const auto & plane=planes.planes[view];
        auto slice=render_itk<Image>(image.GetPointer(),plane.center,plane.u,plane.v,
                                     3,3,1,1,0,Mode::Mean);
        require(slice.pixels.IsNotNull(),"phantom view projection");
        itk::Index<2> middlePixel; middlePixel[0]=1; middlePixel[1]=1;
        require(slice.valid->GetPixel(middlePixel)==1 &&
                approx_equal(slice.pixels->GetPixel(middlePixel),-900),"three views share pivot value");
        if (view!=1) {
            mpr_geometry::Line2D ref;
            require(mpr_geometry::reference_line(planes.planes[view],planes.planes[1],ref),"phantom reference line");
            require(approx_equal(ref.u,0) && approx_equal(ref.v,0),"phantom crosshair at pivot");
        }
    }
    planes.set_pivot({center.x,center.y,center.z+1});
    const auto & moved=planes.planes[1];
    auto movedSlice=render_itk<Image>(image.GetPointer(),moved.center,moved.u,moved.v,1,1,1,1,0,Mode::Mean);
    itk::Index<2> mid; mid.Fill(0);
    require(movedSlice.pixels.IsNotNull() && approx_equal(movedSlice.pixels->GetPixel(mid),-880),"navigation preserves rotated plane");
    // In the three original orientations, positive slider movement must
    // increase the corresponding source voxel index, including coronal Y.
    {
        mpr_geometry::PlaneSet original;
        const Vector3 basis[3]={{0,1,0},{-1,0,0},{0,0,1}};
        original.planes[0]={center,basis[1],basis[2]};
        original.planes[1]={center,basis[0],basis[2]};
        original.planes[2]={center,basis[0],basis[1]};
        for (unsigned a=0; a<3; ++a) {
            original.set_pivot(center);
            const int sign=mpr_geometry::normal_orientation(original.planes[a].u,original.planes[a].v,basis[a]);
            require(advance_pivot_itk(original,a,sign*spacing[a],image.GetPointer()),"positive axis navigation");
            Image::PointType point; point[0]=original.pivot.x; point[1]=original.pivot.y; point[2]=original.pivot.z;
            itk::ContinuousIndex<double,3> voxel; image->TransformPhysicalPointToContinuousIndex(point,voxel);
            require(approx_equal(voxel[a],middle[a]+1),"slider increases original axis");
        }
    }
    // Physical slider extents are derived from the volume box and step size.
    {
        mpr_geometry::PlaneSet bounded;
        const Vector3 basis[3]={{0,1,0},{-1,0,0},{0,0,1}};
        bounded.planes[0]={center,basis[1],basis[2]};
        bounded.planes[1]={center,basis[0],basis[2]};
        bounded.planes[2]={center,basis[0],basis[1]};
        for (unsigned a=0; a<3; ++a) {
            bounded.set_pivot(center);
            const int sign=mpr_geometry::normal_orientation(bounded.planes[a].u,bounded.planes[a].v,basis[a]);
            int low=0,high=0;
            require(navigation_bounds_itk(bounded,a,sign,10,image.GetPointer(),low,high),"axis slider bounds");
            require(low==9 && high==11,"anisotropic axis slider limits");
            require(advance_pivot_itk(bounded,a,sign*spacing[a],image.GetPointer()),"slider endpoint");
            require(navigation_bounds_itk(bounded,a,sign,11,image.GetPointer(),low,high),"endpoint slider bounds");
            require(low==9 && high==11,"slider retains absolute range at endpoint");
        }
        bounded.set_pivot(center);
        require(bounded.rotate_target(2,1,std::acos(-1.0)/6),"rotate bounded plane");
        const int sign=mpr_geometry::normal_orientation(basis[0],basis[2],basis[1]);
        int low=0,high=0;
        require(navigation_bounds_itk(bounded,1,sign,10,image.GetPointer(),low,high),"oblique slider bounds");
        require(low==9 && high==11,"anisotropic oblique slider limits");
    }
    // Navigation uses a physical normal and nonzero ITK region start.
    planes.set_pivot(center);
    require(advance_pivot_itk(planes,2,5.0,image.GetPointer()),"advance along normal");
    require(approx_equal(planes.pivot.z,center.z+5.0),"physical advance distance");
    const auto endpoint=planes.pivot;
    require(!advance_pivot_itk(planes,2,5.0,image.GetPointer()),"volume boundary rejects move");
    require(approx_equal(planes.pivot.z,endpoint.z),"rejected move is atomic");
    require(advance_pivot_itk(planes,2,-5.0,image.GetPointer()),"reverse physical navigation");
    require(approx_equal(planes.pivot.z,center.z),"reverse reaches original pivot");
    require(advance_pivot_itk(planes,1,0.5,image.GetPointer()),"advance oblique normal");
    const auto obliquePivot=planes.pivot;
    require(!approx_equal(obliquePivot.x,center.x) && !approx_equal(obliquePivot.y,center.y),
            "oblique move updates two physical axes");
    require(advance_pivot_itk(planes,1,-0.5,image.GetPointer()),"reverse oblique normal");
    require(approx_equal(planes.pivot.x,center.x) && approx_equal(planes.pivot.y,center.y),
            "oblique navigation reversible");
    // Crosshair click is mapped from the rotated plane to physical space and
    // back to continuous voxel coordinates, without rounding the shared pivot.
    const auto oldPivot=planes.pivot;
    const auto click=mpr_geometry::add(oldPivot,mpr_geometry::multiply(planes.planes[1].u,0.5));
    double relativeVoxel[3]={};
    require(set_pivot_itk(planes,click,image.GetPointer(),relativeVoxel),"physical crosshair click");
    require(approx_equal(planes.pivot.x,click.x) && approx_equal(planes.pivot.y,click.y),"click pivot physical position");
    require(!approx_equal(relativeVoxel[0],std::round(relativeVoxel[0])) ||
            !approx_equal(relativeVoxel[1],std::round(relativeVoxel[1])),"continuous crosshair voxel");
    const auto validPivot=planes.pivot;
    require(!set_pivot_itk(planes,{1e6,1e6,1e6},image.GetPointer(),relativeVoxel),"reject click outside volume");
    require(approx_equal(planes.pivot.x,validPivot.x),"invalid click preserves pivot");
    const auto rejected=render_itk<Image>(image.GetPointer(),center,u,v,100000,100000,2,3,10,Mode::Maximum);
    require(rejected.pixels.IsNull(),"projection work budget");
    auto badBasis=render_itk<Image>(image.GetPointer(),center,u,u,1,1,2,3,4,Mode::Mean);
    require(badBasis.pixels.IsNull(),"invalid basis");
    auto badThickness=render_itk<Image>(image.GetPointer(),center,u,v,1,1,2,3,300000,Mode::Mean);
    require(badThickness.pixels.IsNull(),"sample count limit");
    auto outside=render_itk<Image>(image.GetPointer(),{p[0],p[1],p[2]+100},u,v,1,1,2,3,4,Mode::Mean);
    require(outside.pixels.IsNotNull() && !outside.all_valid && outside.valid->GetPixel(i)==0,"invalid coverage mask");
    auto covered=render_itk<Image>(image.GetPointer(),center,u,v,1,1,2,3,0,Mode::Mean);
    require(covered.all_valid && covered.valid->GetPixel(i)==1,"valid coverage mask");
    std::atomic<bool> cancelled(true);
    auto cancelledResult=render_itk<Image>(image.GetPointer(),center,u,v,1,1,2,3,4,Mode::Mean,&cancelled);
    require(cancelledResult.pixels.IsNull() && cancelledResult.valid.IsNull(),"cancelled projection");
    std::cout << "ITK slab pixels passed\n";
}
