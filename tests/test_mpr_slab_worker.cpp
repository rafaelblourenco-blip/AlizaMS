#include "../GUI/mprslabworker.h"
#include <QCoreApplication>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
static void require(bool ok, const char *message) { if (!ok) { std::cerr << message << '\n'; std::exit(1); } }
int main(int argc, char **argv) {
    QCoreApplication app(argc,argv);
    std::atomic<bool> started(false), stopped(false);
    SlabProjectionWorker worker;
    worker.render = [&](const std::atomic<bool> *cancel) {
        started.store(true);
        while (!cancel->load()) std::this_thread::yield();
        stopped.store(true);
        return mpr_slab::ProjectionImage();
    };
    worker.start();
    for (int n=0; n<5000 && !started.load(); ++n) QThread::msleep(1);
    require(started.load(),"worker start");
    worker.cancelled.store(true);
    require(worker.wait(5000),"worker cancellation and join");
    require(stopped.load() && worker.isFinished(),"worker completion");
    SlabProjectionWorker throwing;
    throwing.render = [](const std::atomic<bool> *) -> mpr_slab::ProjectionImage { throw 1; };
    throwing.start();
    require(throwing.wait(5000) && throwing.result.pixels.IsNull(),"worker exception handling");
    std::cout << "slab worker lifecycle passed\n";
}
