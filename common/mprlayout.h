#ifndef ALIZA_MPR_LAYOUT_H
#define ALIZA_MPR_LAYOUT_H
namespace mpr_layout {
struct Position { int row; int column; };
struct State {
    int grid = 0;
    int maximized = -1;
    void select(int choice) {
        if (choice >= 0 && choice <= 2) { grid = choice; maximized = -1; }
        else if (choice >= 3 && choice <= 5) maximized = choice - 3;
    }
    void double_click(int plane) {
        if (plane >= 0 && plane < 3) maximized = maximized < 0 ? plane : -1;
    }
    bool visible(int plane) const { return maximized < 0 || maximized == plane; }
    Position position(int plane) const {
        return {grid == 1 ? 0 : (grid == 2 ? plane : plane / 2),
                grid == 1 ? plane : (grid == 2 ? 0 : plane % 2)};
    }
};
}
#endif
