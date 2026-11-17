#ifndef ALIZA_MPR_STATE_H
#define ALIZA_MPR_STATE_H
#include "../common/mprgeometry.h"
#include <itkDataObject.h>
class ImageVariant;
// One physical state is shared by the axial, coronal, and sagittal views.
// Enable interactive planes when a compatible volume is initialized.
struct MprSharedState {
    mpr_geometry::PlaneSet planes;
    mpr_geometry::Plane initial[3];
    const ImageVariant *volume{nullptr};
    itk::DataObject::ConstPointer input;
    bool initialized{false};
    bool enabled{false};
    bool lock_orthogonal{false};
    double voxel[3]{0,0,0};
    int sliderStep[3]{0,0,0};
    int initialSliderStep[3]{0,0,0};
    int axisNormalSign[3]{1,1,1};
    void reset(const ImageVariant *image, const itk::DataObject *data, mpr_geometry::Vector3 pivot,
               mpr_geometry::Vector3 e0, mpr_geometry::Vector3 e1, mpr_geometry::Vector3 e2) {
        volume=image;
        input=data;
        planes.planes[0]={pivot,e1,e2};
        planes.planes[1]={pivot,e0,e2};
        planes.planes[2]={pivot,e0,e1};
        axisNormalSign[0]=mpr_geometry::normal_orientation(e1,e2,e0);
        axisNormalSign[1]=mpr_geometry::normal_orientation(e0,e2,e1);
        axisNormalSign[2]=mpr_geometry::normal_orientation(e0,e1,e2);
        planes.set_pivot(pivot);
        for (unsigned i=0; i<3; ++i) initial[i]=planes.planes[i];
        initialized=true;
        enabled=true;
    }
    void reset_position() {
        if (!initialized) return;
        planes.set_pivot(initial[0].center);
        for (unsigned i=0; i<3; ++i) {
            sliderStep[i]=initialSliderStep[i];
            voxel[i]=initialSliderStep[i];
        }
    }
    void reset_orientation() {
        if (!initialized) return;
        const auto pivot=planes.pivot;
        for (unsigned i=0; i<3; ++i) planes.planes[i]=initial[i];
        planes.set_pivot(pivot);
    }
};
#endif
