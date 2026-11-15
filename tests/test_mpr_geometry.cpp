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
    std::cout << "MPR geometry tests passed\n";
}
