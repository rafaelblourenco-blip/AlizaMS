#include "../common/mprgeometry.h"
#include <QTransform>
#include <QPointF>
#include <cmath>
#include <cstdlib>
#include <iostream>

static void require(bool ok, const char *what) { if (!ok) { std::cerr << what << '\n'; std::exit(1); } }
static bool approx_equal(double a, double b) { return std::abs(a-b)<1e-9; }
int main() {
    using namespace mpr_geometry;
    const double pi=std::acos(-1.0);
    const QPointF pivot(125.25, 88.5), first(135.25,88.5), second(125.25,90.5);
    QTransform viewport;
    viewport.translate(320,190);
    viewport.scale(-3.0,2.0);
    viewport.rotate(27.0);
    bool invertible=false;
    const QTransform inverse=viewport.inverted(&invertible);
    require(invertible,"viewport must be invertible");
    const QPointF mappedPivot=inverse.map(viewport.map(pivot));
    const QPointF mappedFirst=inverse.map(viewport.map(first))-mappedPivot;
    const QPointF mappedSecond=inverse.map(viewport.map(second))-mappedPivot;
    const double firstAngle=scene_angle(mappedFirst.x(),mappedFirst.y(),1.0,5.0);
    const double secondAngle=scene_angle(mappedSecond.x(),mappedSecond.y(),1.0,5.0);
    require(approx_equal(secondAngle-firstAngle,pi/2),"flipped, rotated viewport angle with anisotropic spacing");
    PlaneSet planes;
    planes.planes[0]={{0,0,0},{0,1,0},{0,0,1}};
    planes.planes[1]={{0,0,0},{1,0,0},{0,0,1}};
    planes.planes[2]={{0,0,0},{1,0,0},{0,1,0}};
    planes.set_pivot({12,15,18});
    require(planes.rotate_target(2,1,secondAngle-firstAngle),"rotate selected target only");
    require(approx_equal(planes.planes[1].u.x,0) && approx_equal(planes.planes[1].u.y,1),"rotated target orientation");
    require(approx_equal(planes.planes[0].u.y,1),"other plane unchanged");
    Line2D line;
    require(reference_line(planes.planes[2],planes.planes[1],line),"updated reference line");
    require(approx_equal(line.u,0) && approx_equal(line.v,0),"reference crosses shared pivot");
    std::cout << "MPR viewport geometry passed\n";
}
