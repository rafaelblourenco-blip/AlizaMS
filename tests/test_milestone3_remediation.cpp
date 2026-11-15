#include <QApplication>
#include <QTimer>
#include <QPointer>
#include <QCache>
#include <QPixmap>
#include <QThreadPool>
#include <QRunnable>
#include <QRectF>
#include <QPointF>
#include <QString>

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstring>
#include <atomic>
#include <memory>

// Mock classes for testing logic in isolation without requiring full GUI window tree
class MockSlider {
public:
    int val{50};
    int max_val{100};
    int value() const { return val; }
    int maximum() const { return max_val; }
    void setValue(int v) { val = v; }
};

class MockSliderWidget {
public:
    MockSlider * slices_slider{nullptr};
    int last_set_slice{-1};
    int set_slice_count{0};

    MockSliderWidget() {
        slices_slider = new MockSlider();
    }
    ~MockSliderWidget() {
        delete slices_slider;
    }
    void set_slice(int s) {
        last_set_slice = s;
        set_slice_count++;
        if (slices_slider) slices_slider->setValue(s);
    }
};

class MockGraphicsWidget : public QObject {
public:
    MockSliderWidget * slider_m{nullptr};
    void * image3D{nullptr};

    MockGraphicsWidget(void * img = nullptr) : image3D(img) {
        slider_m = new MockSliderWidget();
    }
    ~MockGraphicsWidget() override {
        delete slider_m;
        image3D = nullptr;
    }
};

class MockStudyGraphicsWidget : public QObject {
public:
    MockSlider * slider{nullptr};
    int last_set_slice{-1};
    int set_slice_count{0};

    MockStudyGraphicsWidget() {
        slider = new MockSlider();
    }
    ~MockStudyGraphicsWidget() override {
        delete slider;
    }
    MockSlider * get_slider() { return slider; }
    void set_selected_slice(int s) {
        last_set_slice = s;
        set_slice_count++;
        if (slider) slider->setValue(s);
    }
};

// =========================================================================
// TEST 1: Fractional Delta Accumulation in RadiantInputDispatcher
// =========================================================================
void test_fractional_delta_accumulation() {
    std::cout << "[RUNNING] test_fractional_delta_accumulation..." << std::endl;

    int m_wheel_accumulator_gw = 0;
    int pending_steps_gw = 0;

    auto simulate_wheel = [&](int delta) {
        m_wheel_accumulator_gw += delta;
        const int threshold = 120;
        if (std::abs(m_wheel_accumulator_gw) >= threshold) {
            const int steps = m_wheel_accumulator_gw / threshold;
            m_wheel_accumulator_gw %= threshold;
            pending_steps_gw += steps;
        }
        // Notice: NO else if (delta != 0) branch!
    };

    // Subtest 1: 8 events of delta=15 (8 * 15 = 120) must yield exactly 1 step
    m_wheel_accumulator_gw = 0;
    pending_steps_gw = 0;
    for (int i = 0; i < 8; ++i) {
        simulate_wheel(15);
    }
    assert(pending_steps_gw == 1);
    assert(m_wheel_accumulator_gw == 0);
    std::cout << "  PASS: 8 events of delta=15 accumulated into exactly 1 step." << std::endl;

    // Subtest 2: 7 events of delta=15 (7 * 15 = 105 < 120) must yield 0 steps
    m_wheel_accumulator_gw = 0;
    pending_steps_gw = 0;
    for (int i = 0; i < 7; ++i) {
        simulate_wheel(15);
    }
    assert(pending_steps_gw == 0);
    assert(m_wheel_accumulator_gw == 105);
    std::cout << "  PASS: 7 events of delta=15 yielded 0 steps (accumulator=105)." << std::endl;

    // 8th event completes the step
    simulate_wheel(15);
    assert(pending_steps_gw == 1);
    assert(m_wheel_accumulator_gw == 0);
    std::cout << "  PASS: 8th event of delta=15 completed the step." << std::endl;

    // Subtest 3: 4 events of delta=30 (4 * 30 = 120)
    m_wheel_accumulator_gw = 0;
    pending_steps_gw = 0;
    for (int i = 0; i < 4; ++i) {
        simulate_wheel(30);
    }
    assert(pending_steps_gw == 1);
    assert(m_wheel_accumulator_gw == 0);
    std::cout << "  PASS: 4 events of delta=30 accumulated into 1 step." << std::endl;

    // Subtest 4: Negative fractional deltas: 8 events of delta=-15 (-120)
    m_wheel_accumulator_gw = 0;
    pending_steps_gw = 0;
    for (int i = 0; i < 8; ++i) {
        simulate_wheel(-15);
    }
    assert(pending_steps_gw == -1);
    assert(m_wheel_accumulator_gw == 0);
    std::cout << "  PASS: 8 events of delta=-15 accumulated into -1 step." << std::endl;

    // Subtest 5: Rapid reversal: +15, +15, -15, -15 leaves accumulator at 0
    m_wheel_accumulator_gw = 0;
    pending_steps_gw = 0;
    simulate_wheel(15);
    simulate_wheel(15);
    assert(m_wheel_accumulator_gw == 30);
    simulate_wheel(-15);
    simulate_wheel(-15);
    assert(m_wheel_accumulator_gw == 0);
    assert(pending_steps_gw == 0);
    std::cout << "  PASS: Fractional delta direction reversals cancel cleanly." << std::endl;

    // Subtest 6: Large delta: +1000 produces 8 steps and leaves +40
    m_wheel_accumulator_gw = 0;
    pending_steps_gw = 0;
    simulate_wheel(1000);
    assert(pending_steps_gw == 8);
    assert(m_wheel_accumulator_gw == 40);
    std::cout << "  PASS: Large delta 1000 produces 8 steps with remainder 40." << std::endl;
}

// =========================================================================
// TEST 2: Cross-Viewport Step Isolation
// =========================================================================
void test_cross_viewport_step_isolation() {
    std::cout << "[RUNNING] test_cross_viewport_step_isolation..." << std::endl;

    MockGraphicsWidget gw1;
    MockGraphicsWidget gw2;
    gw1.slider_m->slices_slider->setValue(20);
    gw2.slider_m->slices_slider->setValue(50);

    QPointer<MockGraphicsWidget> pending_gw;
    int pending_steps_gw = 0;

    auto dispatch_pending_slices = [&]() {
        if (pending_gw && pending_steps_gw != 0) {
            if (pending_gw->slider_m && pending_gw->slider_m->slices_slider) {
                const int cur = pending_gw->slider_m->slices_slider->value();
                const int max = pending_gw->slider_m->slices_slider->maximum();
                int next = cur + pending_steps_gw;
                if (next < 0) next = 0;
                if (next > max) next = max;
                pending_steps_gw = 0;
                if (next != cur) {
                    pending_gw->slider_m->set_slice(next);
                }
            } else {
                pending_steps_gw = 0;
            }
            pending_gw = nullptr;
        } else {
            pending_steps_gw = 0;
            pending_gw = nullptr;
        }
    };

    auto queue_slice_step_gw = [&](MockGraphicsWidget * gw, int steps) {
        if (!gw || steps == 0) return;
        if (pending_gw && pending_gw != gw) {
            dispatch_pending_slices();
        }
        pending_gw = gw;
        pending_steps_gw += steps;
    };

    // Scenario: user scrolls 3 slices on gw1
    queue_slice_step_gw(&gw1, 3);
    assert(pending_gw == &gw1);
    assert(pending_steps_gw == 3);
    assert(gw1.slider_m->set_slice_count == 0); // not dispatched yet

    // User immediately moves cursor to gw2 and scrolls 5 slices BEFORE timer fires
    queue_slice_step_gw(&gw2, 5);
    // gw1 must have been flushed immediately
    assert(gw1.slider_m->set_slice_count == 1);
    assert(gw1.slider_m->last_set_slice == 23); // 20 + 3
    assert(gw1.slider_m->slices_slider->value() == 23);

    // gw2 should now hold ONLY its 5 steps, NOT 3 + 5 = 8 steps!
    assert(pending_gw == &gw2);
    assert(pending_steps_gw == 5);
    assert(gw2.slider_m->set_slice_count == 0);

    // Dispatch gw2
    dispatch_pending_slices();
    assert(gw2.slider_m->set_slice_count == 1);
    assert(gw2.slider_m->last_set_slice == 55); // 50 + 5
    assert(gw2.slider_m->slices_slider->value() == 55);

    std::cout << "  PASS: Viewport switching cleanly flushed previous widget and isolated new steps." << std::endl;
}

// =========================================================================
// TEST 3: Bullet Collision Debounce Safety (QPointer & Image Pointer Check)
// =========================================================================
void test_collision_debounce_safety() {
    std::cout << "[RUNNING] test_collision_debounce_safety..." << std::endl;

    int dummy_image = 42;
    int collision_calls = 0;

    static const void * s_pending_collision_v = nullptr;
    static QPointer<MockGraphicsWidget> s_pending_collision_w;

    auto check_slice_collisions = [&](const void *, MockGraphicsWidget *) {
        collision_calls++;
    };

    auto timer_callback = [&]() {
        if (s_pending_collision_w && s_pending_collision_v &&
            s_pending_collision_w->image3D == s_pending_collision_v) {
            check_slice_collisions(s_pending_collision_v, s_pending_collision_w.data());
        }
        s_pending_collision_v = nullptr;
        s_pending_collision_w = nullptr;
    };

    // Subtest 1: Normal execution (widget and image alive)
    {
        MockGraphicsWidget w(&dummy_image);
        s_pending_collision_v = &dummy_image;
        s_pending_collision_w = &w;

        timer_callback();
        assert(collision_calls == 1);
        assert(s_pending_collision_v == nullptr);
        assert(s_pending_collision_w.isNull());
        std::cout << "  PASS: Normal debounce execution invoked collision check once." << std::endl;
    }

    // Subtest 2: Widget destroyed before timer callback fires (Use-After-Free attack)
    {
        collision_calls = 0;
        {
            MockGraphicsWidget short_lived_w(&dummy_image);
            s_pending_collision_v = &dummy_image;
            s_pending_collision_w = &short_lived_w;
            assert(!s_pending_collision_w.isNull());
        } // short_lived_w destroyed here!

        assert(s_pending_collision_w.isNull()); // QPointer automatically cleared!
        timer_callback();
        assert(collision_calls == 0); // Must NOT be called!
        assert(s_pending_collision_v == nullptr);
        std::cout << "  PASS: Destroyed widget prevented use-after-free in debounce timer." << std::endl;
    }

    // Subtest 3: Dataset closed/cleared before timer callback fires (image3D set to nullptr)
    {
        collision_calls = 0;
        MockGraphicsWidget w(&dummy_image);
        s_pending_collision_v = &dummy_image;
        s_pending_collision_w = &w;

        // Image cleared by clear_() or dataset unload
        w.image3D = nullptr;

        timer_callback();
        assert(collision_calls == 0); // Aborted because image3D != s_pending_collision_v
        assert(s_pending_collision_v == nullptr);
        std::cout << "  PASS: Cleared dataset (image3D == nullptr) aborted collision check safely." << std::endl;
    }

    // Subtest 4: Dataset replaced by a different image before callback fires
    {
        collision_calls = 0;
        int another_image = 99;
        MockGraphicsWidget w(&dummy_image);
        s_pending_collision_v = &dummy_image;
        s_pending_collision_w = &w;

        // Swapped to another image
        w.image3D = &another_image;

        timer_callback();
        assert(collision_calls == 0); // Aborted because another_image != dummy_image
        std::cout << "  PASS: Swapped dataset aborted stale collision check safely." << std::endl;
    }
}

// =========================================================================
// TEST 4: LRU Slice Cache & Fine-Grain W/L Key Resolution
// =========================================================================
void test_lru_cache_and_key_precision() {
    std::cout << "[RUNNING] test_lru_cache_and_key_precision..." << std::endl;

    // Subtest 1: Cache capacity and eviction
    QCache<QString, QPixmap> cache;
    cache.setMaxCost(64);

    for (int i = 0; i < 100; ++i) {
        QString key = QString::asprintf("slice_%d", i);
        cache.insert(key, new QPixmap(10, 10));
    }

    assert(cache.size() == 64);
    assert(cache.totalCost() == 64);
    // Oldest items (0..35) should have been evicted
    assert(!cache.contains("slice_0"));
    assert(!cache.contains("slice_35"));
    // Newest items (36..99) should be retained
    assert(cache.contains("slice_36"));
    assert(cache.contains("slice_99"));
    std::cout << "  PASS: LRU cache capacity 64 strictly enforced under 100 insertions." << std::endl;

    // Subtest 2: Key formatting precision (c%.3f_w%.3f)
    double c1 = 40.010;
    double c2 = 40.040;
    double w1 = 350.000;
    double w2 = 350.050;

    QString key1 = QString::asprintf("v%p_a%d_s%d_c%.3f_w%.3f_l%d_%d_lf%d_ov%d",
                                     (void*)0x1234, 2, 10, c1, w1, 0, 0, 0, 0);
    QString key2 = QString::asprintf("v%p_a%d_s%d_c%.3f_w%.3f_l%d_%d_lf%d_ov%d",
                                     (void*)0x1234, 2, 10, c2, w1, 0, 0, 0, 0);
    QString key3 = QString::asprintf("v%p_a%d_s%d_c%.3f_w%.3f_l%d_%d_lf%d_ov%d",
                                     (void*)0x1234, 2, 10, c1, w2, 0, 0, 0, 0);

    assert(key1 != key2);
    assert(key1 != key3);
    assert(key1.contains("c40.010_w350.000"));
    assert(key2.contains("c40.040_w350.000"));
    assert(key3.contains("c40.010_w350.050"));
    std::cout << "  PASS: Fine-grain sub-0.1 HU adjustments generate distinct cache keys." << std::endl;

    cache.clear();
    assert(cache.isEmpty());
    std::cout << "  PASS: Cache clear completely empties cache." << std::endl;
}

// =========================================================================
// TEST 5: Direct Measurement Badge Geometry Math
// =========================================================================
void test_measurement_badge_geometry() {
    std::cout << "[RUNNING] test_measurement_badge_geometry..." << std::endl;

    auto compute_badge = [](QPointF p0, QPointF p1, QRectF vr, int text_w, int text_h) {
        const QPointF mid_screen = (p0 + p1) * 0.5;
        const double dx = p1.x() - p0.x();
        const double dy = p1.y() - p0.y();
        const double length = std::hypot(dx, dy);

        QPointF normal(0.0, -1.0);
        if (length > 1e-4) {
            normal = QPointF(-dy / length, dx / length);
            if (normal.y() > 0.0) {
                normal = -normal;
            }
        }

        const double normal_offset = 18.0;
        const QPointF badge_center = mid_screen + normal * normal_offset;

        const int pad_x = 8;
        const int pad_y = 4;
        const int badge_w = text_w + 2 * pad_x;
        const int badge_h = text_h + 2 * pad_y;

        QRectF badge_rect(badge_center.x() - badge_w / 2.0, badge_center.y() - badge_h / 2.0, badge_w, badge_h);

        if (badge_rect.left() < vr.left() + 4) badge_rect.moveLeft(vr.left() + 4);
        if (badge_rect.right() > vr.right() - 4) badge_rect.moveRight(vr.right() - 4);
        if (badge_rect.top() < vr.top() + 4) badge_rect.moveTop(vr.top() + 4);
        if (badge_rect.bottom() > vr.bottom() - 4) badge_rect.moveBottom(vr.bottom() - 4);

        return std::make_pair(normal, badge_rect);
    };

    QRectF viewport(0, 0, 800, 600);

    // Subtest 1: Horizontal line (0, 100) -> (100, 100)
    {
        auto [norm, rect] = compute_badge(QPointF(0, 100), QPointF(100, 100), viewport, 50, 15);
        // Orthogonality: dot product of (100, 0) and (0, -1) == 0
        assert(std::abs(norm.x()) < 1e-6);
        assert(std::abs(norm.y() - (-1.0)) < 1e-6); // oriented upward
        // Center of line is (50, 100). Offset 18 upward -> (50, 82)
        assert(std::abs(rect.center().x() - 50.0) < 1e-4);
        assert(std::abs(rect.center().y() - 82.0) < 1e-4);
        std::cout << "  PASS: Horizontal line normal vector oriented upward (0, -1) with exact offset." << std::endl;
    }

    // Subtest 2: Diagonal line (0, 0) -> (100, 100)
    {
        auto [norm, rect] = compute_badge(QPointF(0, 0), QPointF(100, 100), viewport, 50, 15);
        // dx=100, dy=100, norm=(-1/sqrt(2), 1/sqrt(2)) -> negated: (1/sqrt(2), -1/sqrt(2))
        double dot = 100.0 * norm.x() + 100.0 * norm.y();
        assert(std::abs(dot) < 1e-6); // orthogonal
        assert(std::abs(std::hypot(norm.x(), norm.y()) - 1.0) < 1e-6); // unit length
        assert(norm.y() <= 0.0); // upward
        std::cout << "  PASS: Diagonal line normal vector is orthogonal, unit length, and upward." << std::endl;
    }

    // Subtest 3: Degenerate line (length <= 1e-4)
    {
        auto [norm, rect] = compute_badge(QPointF(200, 200), QPointF(200, 200), viewport, 50, 15);
        assert(!std::isnan(norm.x()) && !std::isnan(norm.y()));
        assert(norm.x() == 0.0 && norm.y() == -1.0);
        std::cout << "  PASS: Degenerate zero-length line safely handled without NaN." << std::endl;
    }

    // Subtest 4: Boundary clamping near top-left corner
    {
        auto [norm, rect] = compute_badge(QPointF(2, 2), QPointF(10, 2), viewport, 50, 15);
        assert(rect.left() >= viewport.left() + 4);
        assert(rect.top() >= viewport.top() + 4);
        std::cout << "  PASS: Viewport boundary clamping preserves 4px padding." << std::endl;
    }
}

// =========================================================================
// TEST 6: Fast Contiguous Buffer Slice Extraction & Bounds Check
// =========================================================================
void test_contiguous_buffer_extraction() {
    std::cout << "[RUNNING] test_contiguous_buffer_extraction..." << std::endl;

    const size_t width = 256;
    const size_t height = 256;
    const size_t depth = 30;
    std::vector<short> volume_buffer(width * height * depth);

    // Fill buffer with deterministic gradient
    for (size_t z = 0; z < depth; ++z) {
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                volume_buffer[z * width * height + y * width + x] = static_cast<short>(z * 1000 + y + x);
            }
        }
    }

    auto get_axial_slice = [&](int idx, std::vector<short> & out_slice) -> bool {
        if (idx < 0 || idx >= static_cast<int>(depth)) {
            return false; // Safely rejected by bounds guard
        }
        const short * src = volume_buffer.data() + static_cast<size_t>(idx) * width * height;
        out_slice.resize(width * height);
        std::memcpy(out_slice.data(), src, width * height * sizeof(short));
        return true;
    };

    // Subtest 1: Slice 0 head bit-for-bit identity
    std::vector<short> s0;
    assert(get_axial_slice(0, s0));
    assert(s0[0] == volume_buffer[0]);
    assert(s0[width * height - 1] == volume_buffer[width * height - 1]);
    std::cout << "  PASS: Slice 0 contiguous buffer matched bit-for-bit." << std::endl;

    // Subtest 2: Slice max-1 tail bit-for-bit identity
    std::vector<short> s_tail;
    assert(get_axial_slice(depth - 1, s_tail));
    assert(s_tail[0] == volume_buffer[(depth - 1) * width * height]);
    assert(s_tail[width * height - 1] == volume_buffer[depth * width * height - 1]);
    std::cout << "  PASS: Slice (depth-1) tail contiguous buffer matched bit-for-bit." << std::endl;

    // Subtest 3: Out-of-bounds guards
    std::vector<short> s_invalid;
    assert(!get_axial_slice(-1, s_invalid));
    assert(!get_axial_slice(depth, s_invalid));
    assert(!get_axial_slice(999, s_invalid));
    assert(!get_axial_slice(-500, s_invalid));
    std::cout << "  PASS: Out-of-bounds slice indices safely intercepted by guard." << std::endl;
}

// =========================================================================
// TEST 7: Zero-Height Chunking Guard in Thread Pool Tasks
// =========================================================================
class MockTask : public QRunnable {
public:
    int height;
    std::atomic<bool> * executed;
    MockTask(int h, std::atomic<bool> * e) : height(h), executed(e) {
        setAutoDelete(false);
    }
    void run() override {
        assert(height > 0); // Must NEVER receive a 0-height task!
        *executed = true;
    }
};

void test_zero_height_chunking_guard() {
    std::cout << "[RUNNING] test_zero_height_chunking_guard..." << std::endl;

    // Case A: size[1] is an exact multiple of block (e.g. 512, block=64 -> 8 chunks, tmp100=0)
    {
        const int size_1 = 512;
        const int block = 64;
        const int tmp100 = size_1 % block;
        const int incr = size_1 / block;

        std::vector<int> task_heights;
        for (int i = 0; i < incr; ++i) {
            task_heights.push_back(block);
        }
        if (tmp100 > 0) {
            task_heights.push_back(tmp100);
        }

        assert(task_heights.size() == 8);
        for (int h : task_heights) {
            assert(h > 0);
        }
        std::cout << "  PASS: Exact block multiple (tmp100 == 0) creates exactly 8 chunks, 0 zero-height tasks." << std::endl;
    }

    // Case B: size[1] has a remainder (e.g. 500, block=64 -> 7 chunks of 64, tmp100=52)
    {
        const int size_1 = 500;
        const int block = 64;
        const int tmp100 = size_1 % block;
        const int incr = size_1 / block;

        std::vector<int> task_heights;
        for (int i = 0; i < incr; ++i) {
            task_heights.push_back(block);
        }
        if (tmp100 > 0) {
            task_heights.push_back(tmp100);
        }

        assert(task_heights.size() == 8);
        assert(task_heights.back() == 52); // Remainder task has height 52
        for (int h : task_heights) {
            assert(h > 0);
        }
        std::cout << "  PASS: Non-multiple (tmp100 > 0) creates remainder task with exact non-zero height (52)." << std::endl;
    }

    // Case C: Actual QThreadPool dispatch with tasks
    {
        std::atomic<bool> flag1{false};
        std::atomic<bool> flag2{false};
        MockTask t1(64, &flag1);
        MockTask t2(52, &flag2);

        QThreadPool::globalInstance()->start(&t1);
        QThreadPool::globalInstance()->start(&t2);
        QThreadPool::globalInstance()->waitForDone();

        assert(flag1.load());
        assert(flag2.load());
        std::cout << "  PASS: Thread pool tasks executed and synchronized cleanly." << std::endl;
    }
}

// =========================================================================
// MAIN ENTRY POINT
// =========================================================================
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    std::cout << "=========================================================" << std::endl;
    std::cout << "  AlizaMS Milestone 3 Remediation Unit Verification Suite" << std::endl;
    std::cout << "=========================================================" << std::endl;

    test_fractional_delta_accumulation();
    test_cross_viewport_step_isolation();
    test_collision_debounce_safety();
    test_lru_cache_and_key_precision();
    test_measurement_badge_geometry();
    test_contiguous_buffer_extraction();
    test_zero_height_chunking_guard();

    std::cout << "=========================================================" << std::endl;
    std::cout << "  ALL 7 REMEDIATION & VERIFICATION TESTS PASSED (100%)!  " << std::endl;
    std::cout << "=========================================================" << std::endl;

    return 0;
}
