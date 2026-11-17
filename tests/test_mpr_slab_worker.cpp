#include "../GUI/mprslabscheduler.h"
#include <QCoreApplication>
#include <QElapsedTimer>
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
    // A worker closure retains its ITK image after the UI releases its reference.
    {
        using Input=itk::Image<float,3>;
        auto input=Input::New();
        Input::SizeType size; size.Fill(3);
        Input::IndexType index; index.Fill(0);
        Input::RegionType region; region.SetIndex(index); region.SetSize(size);
        input->SetRegions(region); input->Allocate(); input->FillBuffer(42.0f);
        SlabProjectionWorker retaining;
        retaining.render=[input](const std::atomic<bool> *cancel) {
            return mpr_slab::render_itk(input.GetPointer(),{1,1,1},{1,0,0},{0,1,0},
                                        1,1,1,1,0,mpr_slab::Mode::Mean,cancel);
        };
        input=nullptr;
        retaining.start();
        require(retaining.wait(5000) && retaining.result.pixels.IsNotNull(),"retained ITK volume");
        itk::Index<2> pixel; pixel.Fill(0);
        require(retaining.result.pixels->GetPixel(pixel)==42.0,"retained ITK pixel");
    }
    // Exercise the actual GUI scheduler: hold two obsolete requests in flight,
    // replace the queued request, and ensure only the newest result is accepted.
    {
        SlabScheduler scheduler;
        std::atomic<bool> release(false);
        std::atomic<int> running(0);
        int accepted=0, callbacks=0;
        auto submit = [&](int id, bool hold) {
            auto *w = new SlabProjectionWorker();
            w->render = [&, hold](const std::atomic<bool> *) {
                ++running;
                while (hold && !release.load()) std::this_thread::yield();
                return mpr_slab::ProjectionImage();
            };
            scheduler.request(w,[&, id](const mpr_slab::ProjectionImage &) { accepted=id; ++callbacks; });
        };
        submit(1,true);
        for (int n=0; n<5000 && running.load()<1; ++n) QThread::msleep(1);
        require(running.load()==1,"first scheduled worker");
        submit(2,true);
        for (int n=0; n<5000 && running.load()<2; ++n) QThread::msleep(1);
        require(running.load()==2,"second scheduled worker");
        submit(3,false);
        submit(4,false);
        release.store(true);
        QElapsedTimer timer; timer.start();
        while (accepted != 4 && timer.elapsed()<5000) {
            QCoreApplication::processEvents(); QThread::msleep(1);
        }
        require(accepted==4 && callbacks==1 && running.load()==3,"latest request coalescing");
    }
    // A finished signal queued during teardown must not invoke a dead receiver.
    {
        bool called=false;
        auto *scheduler=new SlabScheduler();
        auto *w=new SlabProjectionWorker();
        w->render=[](const std::atomic<bool> *) { return mpr_slab::ProjectionImage(); };
        scheduler->request(w,[&](const mpr_slab::ProjectionImage &) { called=true; });
        require(w->wait(5000),"finished worker before teardown");
        delete scheduler;
        QCoreApplication::processEvents();
        require(!called,"queued completion after teardown");
    }
    // Completion may itself close its owner; the scheduler must not access
    // itself after the callback destroys it.
    {
        bool called=false;
        auto *scheduler=new SlabScheduler();
        auto *w=new SlabProjectionWorker();
        w->render=[](const std::atomic<bool> *) { return mpr_slab::ProjectionImage(); };
        scheduler->request(w,[&](const mpr_slab::ProjectionImage &) {
            called=true; delete scheduler; scheduler=nullptr;
        });
        QElapsedTimer timer; timer.start();
        while (!called && timer.elapsed()<5000) { QCoreApplication::processEvents(); QThread::msleep(1); }
        require(called && scheduler==nullptr,"scheduler deletion in completion");
    }
    std::cout << "slab worker lifecycle passed\n";
}
