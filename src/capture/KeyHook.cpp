#include "KeyHook.h"
#include "Logger.h"

namespace TechVideoEditor {

KeyHook::KeyHook() = default;

KeyHook::~KeyHook() {
    Remove();
}

bool KeyHook::Install() {
    if (m_installed) {
        return true;
    }
    
    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, nullptr, 0);
    if (!m_hook) {
        LOG_ERROR("Failed to install keyboard hook: " + std::to_string(GetLastError()));
        return false;
    }
    
    m_installed = true;
    LOG_INFO("Keyboard hook installed successfully");
    return true;
}

void KeyHook::Remove() {
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
        m_installed = false;
        LOG_INFO("Keyboard hook removed");
    }
}

LRESULT CALLBACK KeyHook::HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pKbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        
        if (isKeyDown || isKeyUp) {
            // Process in main thread via callback
            // We can't do heavy processing here as it blocks the input chain
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void KeyHook::ProcessKeyEvent(int keyCode, bool isPressed) {
    if (isPressed) {
        m_activeKeys.insert(keyCode);
        
        // Track modifiers
        switch (keyCode) {
            case VK_CONTROL:
            case VK_LCONTROL:
            case VK_RCONTROL:
                m_ctrlPressed = true;
                break;
            case VK_MENU:
            case VK_LMENU:
            case VK_RMENU:
                m_altPressed = true;
                break;
            case VK_SHIFT:
            case VK_LSHIFT:
            case VK_RSHIFT:
                m_shiftPressed = true;
                break;
            case VK_LWIN:
            case VK_RWIN:
                m_winPressed = true;
                break;
        }
        
        // Auto-capture combinations
        if (m_autoCapture && m_overlayEnabled) {
            KeyCombination combo;
            
            if (m_ctrlPressed) combo.keys.push_back(VK_CONTROL);
            if (m_altPressed) combo.keys.push_back(VK_MENU);
            if (m_shiftPressed) combo.keys.push_back(VK_SHIFT);
            if (m_winPressed) combo.keys.push_back(VK_LWIN);
            
            // Add the non-modifier key
            bool isModifier = (keyCode == VK_CONTROL || keyCode == VK_MENU || 
                              keyCode == VK_SHIFT || keyCode == VK_LWIN || keyCode == VK_RWIN);
            
            if (!isModifier) {
                combo.keys.push_back(keyCode);
                
                // Build display string
                std::string display;
                for (size_t i = 0; i < combo.keys.size(); ++i) {
                    if (i > 0) display += " + ";
                    display += GetKeyDisplayString(combo.keys[i]);
                }
                
                combo.displayText = display;
                m_recentCombinations.push_back(combo);
                
                // Keep only last 10 combinations
                if (m_recentCombinations.size() > 10) {
                    m_recentCombinations.erase(m_recentCombinations.begin());
                }
            }
        }
    } else {
        m_activeKeys.erase(keyCode);
        
        // Update modifiers
        switch (keyCode) {
            case VK_CONTROL:
            case VK_LCONTROL:
            case VK_RCONTROL:
                m_ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                break;
            case VK_MENU:
            case VK_LMENU:
            case VK_RMENU:
                m_altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                break;
            case VK_SHIFT:
            case VK_LSHIFT:
            case VK_RSHIFT:
                m_shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                break;
            case VK_LWIN:
            case VK_RWIN:
                m_winPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || 
                              (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
                break;
        }
    }
    
    if (m_keyCallback) {
        m_keyCallback(keyCode, isPressed);
    }
}

std::vector<KeyCombination> KeyHook::GetRecentCombinations(int seconds) const {
    // In a real implementation, we'd filter by timestamp
    return m_recentCombinations;
}

std::string KeyHook::GetKeyDisplayString(int keyCode) const {
    switch (keyCode) {
        case VK_CONTROL: return "Ctrl";
        case VK_MENU: return "Alt";
        case VK_SHIFT: return "Shift";
        case VK_LWIN:
        case VK_RWIN: return "Win";
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        case VK_ESCAPE: return "Esc";
        case VK_TAB: return "Tab";
        case VK_BACK: return "Backspace";
        case VK_DELETE: return "Del";
        case VK_INSERT: return "Ins";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "PgUp";
        case VK_NEXT: return "PgDn";
        case VK_LEFT: return "←";
        case VK_UP: return "↑";
        case VK_RIGHT: return "→";
        case VK_DOWN: return "↓";
        default:
            if (keyCode >= 'A' && keyCode <= 'Z') {
                return std::string(1, static_cast<char>(keyCode));
            }
            if (keyCode >= '0' && keyCode <= '9') {
                return std::string(1, static_cast<char>(keyCode));
            }
            if (keyCode >= VK_F1 && keyCode <= VK_F24) {
                return "F" + std::to_string(keyCode - VK_F1 + 1);
            }
            return "Key" + std::to_string(keyCode);
    }
}

void KeyHook::UpdateActiveModifiers() {
    m_ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    m_altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    m_shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    m_winPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || 
                  (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
}

} // namespace TechVideoEditor
