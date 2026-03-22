#include "input/InputManager.h"

namespace sculpt {

void InputManager::onPointerEvent(const PointerEvent& event) {
    m_pointerX = event.x;
    m_pointerY = event.y;

    if (event.button >= 0 && event.button < 3) {
        if (event.action == PointerAction::Press) {
            m_buttons[event.button] = true;
        } else if (event.action == PointerAction::Release) {
            m_buttons[event.button] = false;
        }
    }

    if (m_pointerCb) {
        m_pointerCb(event);
    }
}

void InputManager::onScrollEvent(const ScrollEvent& event) {
    if (m_scrollCb) {
        m_scrollCb(event);
    }
}

bool InputManager::isPressed(int button) const {
    if (button >= 0 && button < 3) {
        return m_buttons[button];
    }
    return false;
}

} // namespace sculpt
