#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <imgui.h>
#include <vector>
#include <memory>
#include <string>

#include "ScreenCapture.h"
#include "Timeline.h"
#include "AudioMixer.h"
#include "KeyHook.h"

namespace TechVideoEditor {

class UIManager {
public:
    UIManager();
    ~UIManager();

    // Initialize with D3D11 device and window
    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    
    // Main render loop (call each frame)
    void BeginFrame();
    void EndFrame();
    
    // Render main editor UI
    void RenderEditorUI();
    
    // Render individual panels
    void RenderMenuBar();
    void RenderToolbar();
    void RenderPreviewWindow();
    void RenderTimelinePanel();
    void RenderTracksPanel();
    void RenderPropertiesPanel();
    void RenderKeyPressOverlay();
    
    // Get/set UI state
    bool IsPreviewWindowFocused() const { return m_previewFocused; }
    void SetPreviewFocused(bool focused) { m_previewFocused = focused; }
    
    // Docking space for ImGui windows
    ImGuiID GetDockSpaceID() const { return m_dockSpaceID; }
    
    // Theme
    void ApplyDarkTheme();

private:
    void RenderClipItem(const TimelineClip& clip, float trackHeight);
    void RenderPlayhead(int64_t startTime, int64_t endTime);
    void RenderZoomControls();
    void RenderExportDialog();
    
    HWND m_hwnd;
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    
    bool m_initialized = false;
    bool m_previewFocused = false;
    bool m_showExportDialog = false;
    
    ImGuiID m_dockSpaceID;
    
    // UI state
    float m_timelineScroll;
    int m_selectedTrack;
    std::string m_exportPath;
    
    // Preview texture
    ComPtr<ID3D11ShaderResourceView> m_previewSRV;
    uint32_t m_previewWidth;
    uint32_t m_previewHeight;
};

} // namespace TechVideoEditor
