#ifndef ALIZA_MPR_SLAB_WORKER_H
#define ALIZA_MPR_SLAB_WORKER_H
#include "../common/mprslabitk.h"
#include <QThread>
#include <atomic>
#include <functional>

// No widget or ImageVariant pointer is accessed from this worker. The render
// closure retains its ITK input and the owner cancels and joins before teardown.
class SlabProjectionWorker : public QThread
{
public:
    std::atomic<bool> cancelled{false};
    unsigned long long generation{0};
    unsigned int width{0}, height{0};
    std::function<void(const mpr_slab::ProjectionImage &)> completion;
    std::function<mpr_slab::ProjectionImage(const std::atomic<bool> *)> render;
    mpr_slab::ProjectionImage result;
    void run() override {
        try { if (render && !cancelled.load()) result = render(&cancelled); }
        catch (...) { result = mpr_slab::ProjectionImage(); }
    }
};
#endif
