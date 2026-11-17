#include "../common/mprmeasurementgeometry.h"
#include <cstdlib>
#include <iostream>
static void require(bool ok,const char *why) { if (!ok) { std::cerr<<why<<'\n'; std::exit(1); } }
static bool near(double a,double b) { return std::abs(a-b)<1e-6; }
int main() {
    using namespace mpr_measurement;
    require(near(angle_degrees({1,1},{0,0},{1,0},2,1),26.565051177),"physical angle with anisotropic spacing");
    require(angle_degrees({0,0},{0,0},{1,0},1,1)<0,"degenerate angle");
    require(near(ellipse_area(QRectF(0,0,10,20),2,0.5),50*std::acos(-1.0)),"physical ellipse area");
    require(ellipse_area(QRectF(0,0,0,20),1,1)<0,"degenerate ellipse");
    QPainterPath square; square.addRect(QRectF(0,0,10,20)); double area=0;
    require(simple_area(square,2,0.5,area) && near(area,200),"physical polygon area");
    QVector<QPointF> corners{{0,0},{10,0},{10,20},{0,20}};
    auto rounded=rounded_polygon(corners);
    require(simple_area(rounded,2,0.5,area) && area>0 && area<200,"rounded area uses displayed boundary");
    QPainterPath crossed; crossed.moveTo(0,0); crossed.lineTo(10,10); crossed.lineTo(0,10); crossed.lineTo(10,0); crossed.closeSubpath();
    require(!simple_area(crossed,1,1,area),"reject self-intersection");
    QVector<QPointF> segment{{0,0},{10,0}};
    require(!valid_segments(segment,[](QPointF p){return p.x()<4 || p.x()>6;}),"detect invalid segment interior");
    require(valid_segments(segment,[](QPointF){return true;}),"valid segment");
    require(!simple_area(square,0,1,area),"reject invalid spacing");
    std::cout<<"MPR measurement geometry passed\n";
}
