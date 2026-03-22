#pragma once

namespace sculpt {

enum class AppMode { Sculpt, Edit, Paint, Pose };

struct ModeManager {
    AppMode current = AppMode::Sculpt;

    void toggle() {
        if (current == AppMode::Sculpt) current = AppMode::Edit;
        else if (current == AppMode::Edit) current = AppMode::Paint;
        else if (current == AppMode::Paint) current = AppMode::Pose;
        else current = AppMode::Sculpt;
    }
    bool isSculpt() const { return current == AppMode::Sculpt; }
    bool isEdit() const { return current == AppMode::Edit; }
    bool isPaint() const { return current == AppMode::Paint; }
    bool isPose() const { return current == AppMode::Pose; }
};

} // namespace sculpt
