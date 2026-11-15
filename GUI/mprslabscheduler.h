#ifndef ALIZA_MPR_SLAB_SCHEDULER_H
#define ALIZA_MPR_SLAB_SCHEDULER_H
#include "mprslabworker.h"
#include <QObject>
#include <QList>
#include <QPointer>
#include <functional>
#include <utility>

// This object is owned and used exclusively by its QObject/GUI thread.
// Worker closures must retain their input buffers for their entire lifetime.
class SlabScheduler : public QObject
{
public:
    using Completion = std::function<void(const mpr_slab::ProjectionImage &)>;
    explicit SlabScheduler(QObject *parent = nullptr) : QObject(parent) {}
    ~SlabScheduler() override {
        cancel();
        for (auto *worker : workers_) { worker->wait(); delete worker; }
        workers_.clear();
    }
    void cancel() {
        ++generation_;
        for (auto *worker : workers_) worker->cancelled.store(true);
        delete queued_;
        queued_ = nullptr;
    }
    // Takes ownership, retaining only the newest queued request.
    void request(SlabProjectionWorker *worker, Completion completion) {
        cancel();
        worker->generation = generation_;
        worker->completion = std::move(completion);
        if (workers_.size() >= 2) queued_ = worker;
        else start(worker);
    }
private:
    void start(SlabProjectionWorker *worker) {
        workers_.append(worker);
        connect(worker, &QThread::finished, this, [this, worker]() {
            workers_.removeOne(worker);
            const bool current = !worker->cancelled.load() && worker->generation == generation_;
            const auto completion = worker->completion;
            const auto result = worker->result;
            worker->deleteLater();
            QPointer<SlabScheduler> self(this);
            if (current && completion) completion(result);
            if (!self) return;
            if (queued_ && workers_.size() < 2) {
                SlabProjectionWorker *next = queued_;
                queued_ = nullptr;
                start(next);
            }
        });
        worker->start();
    }
    QList<SlabProjectionWorker*> workers_;
    SlabProjectionWorker *queued_{nullptr};
    unsigned long long generation_{0};
};
#endif
