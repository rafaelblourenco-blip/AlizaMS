#include "../common/reference_batch.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace mpr_geometry;
static void require(bool ok,const char *msg) { if (!ok) { std::cerr<<msg<<'\n'; std::exit(1); } }
static bool equal(double a,double b) { return std::abs(a-b)<1e-8; }
int main() {
    const Vector3 corners[4]={{0,0,0},{10,0,0},{10,20,0},{0,20,0}};
    PlaneRectangle reconstructed;
    require(rectangle_from_corners(corners,reconstructed),"DICOM corners");
    require(equal(reconstructed.u_max,10)&&equal(reconstructed.v_max,20),"physical corner lengths");
    const Vector3 malformed[4]={{0,0,0},{10,0,0},{11,20,0},{0,20,0}};
    require(!rectangle_from_corners(malformed,reconstructed),"malformed corners");
    const PlaneRectangle view={{{10,20,30},{1,0,0},{0,1,0}},-20,20,-10,10};
    PlaneRectangle slice={{{12,20,30},{0,1,0},{0,0,1}},-5,5,-2,2};
    ReferenceSegment s;
    require(bounded_reference_segment(view,slice,s),"intersection");
    require(equal(s.start.x,12)&&equal(s.end.x,12)&&equal(std::abs(s.end.y-s.start.y),10),"slice bounds");
    slice.plane.center.x=31;
    require(!bounded_reference_segment(view,slice,s),"outside viewport");
    slice.plane.center.x=10; slice.plane.u={1,0,0}; slice.plane.v={0,1,0};
    require(!bounded_reference_segment(view,slice,s),"parallel planes");
    const double q=std::sqrt(0.5);
    slice={{{10,20,30},{q,q,0},{0,0,1}},-5,5,-2,2};
    require(bounded_reference_segment(view,slice,s),"oblique intersection");
    require(equal(std::hypot(s.end.x-s.start.x,s.end.y-s.start.y),10),"oblique physical extent");
    slice.v_min=3; slice.v_max=5;
    require(!bounded_reference_segment(view,slice,s),"disjoint slice bounds");
    slice.plane.u={0,0,0};
    require(!bounded_reference_segment(view,slice,s),"invalid basis");
    slice={{{10,20,30},{0,1,0},{0,0,1}},-5,5,-2,2};
    std::vector<PlaneRectangle> series(7,slice);
    ReferenceBatch batch(view,series);
    require(batch.next(0).empty() && !batch.done(),"zero budget");
    require(batch.next(3).size()==3 && !batch.done(),"bounded first batch");
    require(batch.next(3).size()==3 && !batch.done(),"bounded second batch");
    require(batch.next(3).size()==1 && batch.done(),"bounded final batch");
    std::cout<<"reference segments passed\n";
}
