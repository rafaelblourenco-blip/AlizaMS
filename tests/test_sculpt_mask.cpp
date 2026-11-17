#include "CG/sculptmask.h"
#include <iostream>
#define CHECK(value) do { if (!(value)) { std::cerr << "failed at line " << __LINE__ << "\n"; return 1; } } while (false)

int main()
{
    QMatrix4x4 identity;
    const QPolygonF square(QVector<QPointF>{QPointF(0,0), QPointF(0.5,0), QPointF(0.5,1), QPointF(0,1)});
    SculptMask inside;
    CHECK(inside.apply(4, 1, 1, identity, square, true));
    CHECK((inside.data() == std::vector<unsigned char>{0, 0, 255, 255}));
    SculptMask outside;
    CHECK(outside.apply(4, 1, 1, identity, square, false));
    CHECK((outside.data() == std::vector<unsigned char>{255, 255, 0, 0}));
    CHECK(outside.apply(4, 1, 1, identity, square, true));
    CHECK((outside.data() == std::vector<unsigned char>{0, 0, 0, 0}));
    // Outline remains in the camera coordinate system at the time of the cut.
    QMatrix4x4 flipX;
    flipX.translate(1.0f, 0.0f, 0.0f);
    flipX.scale(-1.0f, 1.0f, 1.0f);
    SculptMask flipped;
    CHECK(flipped.apply(4, 1, 1, flipX, square, true));
    CHECK((flipped.data() == std::vector<unsigned char>{255, 255, 0, 0}));
    const QPolygonF invalid(QVector<QPointF>{QPointF(0,0), QPointF(1,0), QPointF(2,0)});
    CHECK(!inside.apply(4, 1, 1, identity, invalid, false));
    CHECK((inside.data() == std::vector<unsigned char>{0, 0, 255, 255}));
    CHECK(!inside.apply(8, 1, 1, identity, square, true));
    return 0;
}
