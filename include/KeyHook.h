#pragma once

#include <Windows.h>
#include <functional>
#include <string>
#include <set>

namespace TechVideoEditor {

struct KeyState {
    int keyCode;
    bool isPressed;
    bool wasPressed;      // Previous state for edge detection
    uint64_t pressTime;   // Timestamp when pressed
};

struct KeyCombination {
    std::vector<int> keys;  // e.g., {VK_CONTROL, 'C'}
    std::string displayText; // e.g., "Ctrl+C"
};

class KeyHook {
public:
    KeyHook();
    ~KeyHook();

    // Install the low-level keyboard hook
    bool Install();
    
    // Remove the hook
    void Remove();
    
    // Check if hook is active
    bool IsInstalled() const { return m_installed; }
    
    // Enable/disable key overlay rendering
    void SetOverlayEnabled(bool enabled) { m_overlayEnabled = enabled; }
    bool IsOverlayEnabled() const { return m_overlayEnabled; }
    
    // Get currently pressed keys (for overlay rendering)
    const std::set<int>& GetActiveKeys() const { return m_activeKeys; }
    
    // Get recent key combinations (last N seconds)
    std::vector<KeyCombination> GetRecentCombinations(int seconds = 3) const;
    
    // Callback for key events
    using KeyCallback = std::function<void(int keyCode, bool isPressed)>;
    void SetKeyCallback(KeyCallback callback) { m_keyCallback = callback; }
    
    // Auto-capture common modifier combinations
    void EnableAutoCapture(bool enable) { m_autoCapture = enable; }

private:
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
    void ProcessKeyEvent(int keyCode, bool isPressed);
    void UpdateActiveModifiers();
    std::string GetKeyDisplayString(int keyCode) const;
    
    HHOOK m_hook = nullptr;
    bool m_installed = false;
    bool m_overlayEnabled = true;
    bool m_autoCapture = true;
    
    std::set<int> m_activeKeys;
    std::vector<KeyCombination> m_recentCombinations;
    
    KeyCallback m_keyCallback;
    
    // Modifier keys
    bool m_ctrlPressed = false;
    bool m_altPressed = false;
    bool m_shiftPressed = false;
    bool m_winPressed = false;
};

} // namespace TechVideoEditor
