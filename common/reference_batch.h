#ifndef ALIZA_REFERENCE_BATCH_H
#define ALIZA_REFERENCE_BATCH_H
#include "reference_segment.h"
#include <vector>
#include <cstddef>
#include <utility>
namespace mpr_geometry {
// Caller owns the snapshot and schedules each bounded batch on its event loop
// or worker. Replacing a request discards this object and its pending work.
class ReferenceBatch {
public:
    ReferenceBatch(PlaneRectangle view, std::vector<PlaneRectangle> slices)
        : view_(view), slices_(std::move(slices)), next_(0) {}
    bool done() const { return next_ == slices_.size(); }
    std::vector<ReferenceSegment> next(std::size_t budget) {
        std::vector<ReferenceSegment> out;
        while (budget && next_ < slices_.size()) {
            ReferenceSegment segment;
            if (bounded_reference_segment(view_,slices_[next_],segment)) out.push_back(segment);
            ++next_; --budget;
        }
        return out;
    }
private:
    PlaneRectangle view_;
    std::vector<PlaneRectangle> slices_;
    std::size_t next_;
};
}
#endif
