#include "../common/mprgeometry.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace mpr_geometry;
static void require(bool b, const char* what) { if (!b) { std::cerr << what << '\n'; std::exit(1); } }
static bool close(double a, double b) { return std::abs(a-b) < 1e-10; }
static void same(Vector3 a, Vector3 b, const char* msg) { require(close(a.x,b.x)&&close(a.y,b.y)&&close(a.z,b.z),msg); }
int main() {
    const double pi=std::acos(-1.0);
    same(rotate({1,0,0},{0,0,1},pi/2),{0,1,0},"axial rotation");
    same(rotate({0,0,1},{0,-1,0},pi/2),{-1,0,0},"coronal rotation");
    same(rotate({0,1,0},{1,0,0},pi/2),{0,0,1},"sagittal rotation");
    Vector3 axis={0,0,1}, u=rotate({1,0,0},axis,0.63), v=rotate({0,1,0},axis,0.63);
    require(close(dot(u,u),1)&&close(dot(v,v),1)&&close(dot(u,v),0),"orthonormal basis");
    require(close(scene_angle(10,10,0.7,5.0),std::atan2(50.0,7.0)),"anisotropic angle");
    // A reflected image direction changes the handedness of its displayed basis.
    const Vector3 reflected_u={-1,0,0}, reflected_v={0,1,0};
    const Vector3 reflected_normal=cross(reflected_u,reflected_v);
    same(rotate(reflected_u,reflected_normal,pi/2),reflected_v,"reflected direction rotation");
    const Vector3 pivot={32.0,45.0,-9.0};
    const Vector3 center=output_center(pivot,u,v,13,87,256,128,0.7,5.0);
    same(pixel_position(center,u,v,13,87,256,128,0.7,5.0),pivot,"pivot preservation");
    same(pixel_position(center,u,v,14,87,256,128,0.7,5.0),add(pivot,multiply(u,0.7)),"pixel spacing");
    // Plane intersection is in physical millimetres even when the output
    // has unequal pixel spacing or is translated from the source origin.
    const Plane axial{{10,20,30},{1,0,0},{0,1,0}};
    const Plane sagittal{{13,20,30},{0,1,0},{0,0,1}};
    Line2D line;
    require(reference_line(axial,sagittal,line),"orthogonal reference line");
    require(close(line.u,3) && close(line.v,0) && close(std::abs(line.dv),1),"translated intersection");
    const Vector3 obliqueNormal=rotate({1,0,0},{0,0,1},pi/4);
    const Plane oblique{{13,20,30},rotate({0,1,0},{0,0,1},pi/4),{0,0,1}};
    require(reference_line(axial,oblique,line),"oblique reference line");
    for (double t : {-7.0,0.0,9.0}) {
        const double x=line.u+t*line.du, y=line.v+t*line.dv;
        require(close(dot(obliqueNormal, add(add(axial.center,multiply(axial.u,x)),
                                             add(multiply(axial.v,y),multiply(oblique.center,-1)))),0),
                "oblique intersection lies on target plane");
    }
    require(!reference_line(axial,axial,line),"parallel planes do not define line");
    PlaneSet planes;
    planes.planes[0] = {{0,0,0},{0,1,0},{0,0,1}};
    planes.planes[1] = {{0,0,0},{1,0,0},{0,0,1}};
    planes.planes[2] = {{0,0,0},{1,0,0},{0,1,0}};
    planes.set_pivot({3,8,12});
    require(planes.rotate_target(2,1,pi/6),"rotate coronal in axial view");
    for (unsigned view=0; view<3; ++view) {
        same(planes.planes[view].center,{3,8,12},"shared pivot");
        require(close(dot(planes.planes[view].u,planes.planes[view].v),0),"plane orthogonality");
    }
    require(reference_line(planes.planes[2],planes.planes[1],line),"rotated plane reference");
    require(close(line.u,0) && close(line.v,0),"reference goes through shared pivot");
    require(!planes.rotate_target(2,2,pi/4),"reject rotating current plane around itself");
    planes.set_pivot({9,-2,5});
    require(reference_line(planes.planes[0],planes.planes[1],line),"reference after navigation");
    require(close(line.u,0) && close(line.v,0),"navigation preserves shared intersection");
    std::cout << "MPR geometry tests passed\n";
}
