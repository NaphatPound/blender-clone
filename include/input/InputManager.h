#pragma once

#include "core/HalfEdgeMesh.h"
#include <functional>
#include <cstdint>

namespace sculpt {

enum class PointerAction {
    Press,
    Release,
    Move
};

struct PointerEvent {
    float x, y;
    PointerAction action;
    int button;  // 0=left, 1=middle, 2=right
    bool shift;
    bool ctrl;
    bool alt;
};

struct ScrollEvent {
    float deltaX, deltaY;
};

using PointerCallback = std::function<void(const PointerEvent&)>;
using ScrollCallback = std::function<void(const ScrollEvent&)>;

class InputManager {
public:
    InputManager() = default;

    void setPointerCallback(PointerCallback cb) { m_pointerCb = cb; }
    void setScrollCallback(ScrollCallback cb) { m_scrollCb = cb; }

    // Process a pointer event (mouse or touch)
    void onPointerEvent(const PointerEvent& event);
    void onScrollEvent(const ScrollEvent& event);

    // State queries
    bool isPressed(int button) const;
    float getPointerX() const { return m_pointerX; }
    float getPointerY() const { return m_pointerY; }

private:
    PointerCallback m_pointerCb;
    ScrollCallback m_scrollCb;
    float m_pointerX = 0, m_pointerY = 0;
    bool m_buttons[3] = {false, false, false};
};

} // namespace sculpt
