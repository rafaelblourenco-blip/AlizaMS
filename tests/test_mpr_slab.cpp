#include "../common/mprslab.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace mpr_slab;
static void require(bool ok, const char * message) { if (!ok) { std::cerr << message << '\n'; std::exit(1); } }
static bool approx_equal(double a, double b) { return std::abs(a-b) < 1e-9; }
int main()
{
    // Analytical phantom in physical mm; outside points must never become zero.
    auto ct = [](Vector3 p, double & value) {
        if (p.z < -1 || p.z > 0) return false;
        value = -900 + 100 * p.z;
        return true;
    };
    const Vector3 c{0,0,0}, z{0,0,3}; // non-unit normal exercises normalization
    auto a = project(c,z,4,1,Mode::Maximum,ct);
    auto b = project(c,z,4,1,Mode::Minimum,ct);
    auto d = project(c,z,4,1,Mode::Mean,ct);
    require(a.valid && a.samples == 2 && approx_equal(a.value,-900), "CT edge maximum");
    require(b.valid && b.samples == 2 && approx_equal(b.value,-1000), "CT edge minimum");
    require(d.valid && d.samples == 2 && approx_equal(d.value,-950), "CT edge mean");
    auto thin = project(c,z,0,0.7,Mode::Mean,ct);
    require(thin.valid && thin.samples == 1 && approx_equal(thin.value,-900), "thin center");
    auto none = project(Vector3{0,0,10},z,2,1,Mode::Minimum,ct);
    require(!none.valid && none.samples == 0, "no valid samples");
    // Analytic linear field and oblique unit direction in physical coordinates.
    auto ramp = [](Vector3 p, double & value) { value = 3*p.x - 2*p.y + p.z; return true; };
    const double q = std::sqrt(0.5);
    auto oblique = project(Vector3{4,-2,7},Vector3{q,0,q},4,0.7,Mode::Maximum,ramp);
    const double middle = 3*4 + 4 + 7;
    require(oblique.valid && oblique.samples == 7 && approx_equal(oblique.value,middle+8*q), "oblique physical thickness");
    auto mean = project(Vector3{4,-2,7},Vector3{q,0,q},4,0.7,Mode::Mean,ramp);
    require(mean.valid && approx_equal(mean.value,middle), "symmetric mean");
    // Physical-to-index mapping for a volume rotated 90 degrees, with
    // voxel spacing (2, 3, 5) mm and a linear field in index space.
    auto anisotropic = [](Vector3 p, double & value) {
        const double ix = p.y / 2, iy = -p.x / 3, iz = p.z / 5;
        if (ix < 0 || ix > 2 || iy < 0 || iy > 2 || iz < 0 || iz > 2) return false;
        value = ix + 10 * iy + 100 * iz;
        return true;
    };
    auto physical = project(Vector3{-3,2,5},z,10,5,Mode::Maximum,anisotropic);
    require(physical.valid && physical.samples == 3 && approx_equal(physical.value,211), "anisotropic rotated volume");
    auto constant = [](Vector3, double & value) { value = -300; return true; };
    auto exact = project(c,z,5,2,Mode::Mean,constant);
    require(exact.valid && exact.samples == 5 && approx_equal(exact.value,-300), "physical step and constant");
    require(!project(c,Vector3{0,0,0},1,1,Mode::Mean,constant).valid, "zero normal");
    require(!project(c,z,-1,1,Mode::Mean,constant).valid, "negative thickness");
    require(!project(c,z,1,0,Mode::Mean,constant).valid, "zero step");
    require(!project(c,z,std::numeric_limits<double>::infinity(),1,Mode::Mean,constant).valid, "infinite thickness");
    const double tiny = std::numeric_limits<double>::denorm_min();
    bool finite_points = true;
    auto check_points = [&finite_points](Vector3 p, double & value) {
        finite_points = finite_points && std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
        value = p.z;
        return true;
    };
    auto subnormal = project(c,Vector3{0,0,tiny},2,1,Mode::Maximum,check_points);
    require(finite_points && subnormal.valid && approx_equal(subnormal.value,1), "subnormal normal");
    finite_points = true;
    auto overflow = project(Vector3{(std::numeric_limits<double>::max)(),0,0},
                            Vector3{1,0,0},(std::numeric_limits<double>::max)(),
                            (std::numeric_limits<double>::max)(),Mode::Maximum,check_points);
    require(finite_points && !overflow.valid, "overflow coordinates rejected");
    auto invalid = [](Vector3, double & value) { value = std::numeric_limits<double>::quiet_NaN(); return true; };
    require(!project(c,z,1,1,Mode::Mean,invalid).valid, "nonfinite samples");
    std::cout << "MPR slab tests passed\n";
}
