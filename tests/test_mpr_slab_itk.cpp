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
static bool near(double a, double b) { return std::abs(a-b) < 1e-7; }
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
    require(near(value(Mode::Maximum,10),-800),"ITK MIP");
    require(near(value(Mode::Minimum,10),-1000),"ITK MinIP");
    require(near(value(Mode::Mean,10),-900),"ITK mean");
    require(near(value(Mode::Mean,0),-900),"ITK thin");
    const double q=std::sqrt(0.5);
    // Cross(u,v)=(q,0,q), testing oblique interpolation in physical space.
    const Vector3 ou{0,1,0}, ov{-q,0,q};
    auto oblique=render_itk<Image>(image.GetPointer(),center,ou,ov,1,1,2,3,4,Mode::Maximum);
    require(oblique.pixels.IsNotNull() && oblique.all_valid,"ITK oblique allocation");
    itk::Index<2> i; i.Fill(0);
    require(near(oblique.pixels->GetPixel(i),-900+40*q),"ITK oblique interpolation");
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
    std::cout << "ITK slab pixels passed\n";
}
